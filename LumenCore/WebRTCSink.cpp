#ifdef LUMEN_WITH_WEBRTC
#include "WebRTCSink.h"
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace rtc;

namespace {
	constexpr uint32_t kSsrcValue = 42;
	constexpr uint8_t H264_PAYLOAD_TYPE = 96;

	// FRC field networks only pass ports 5800-5810 for team use; libdatachannel's default is any
	// OS-assigned ephemeral UDP port (1024-65535), which a real field would simply drop - live
	// preview would work on the bench and silently fail at competition. 5800 is the web server
	// (HTTP/WebSocket/MJPEG) and 5810 is the roboRIO's NT4 server, so media gets the slice in
	// between. With the ICE UDP mux (libjuice), the first peer connection binds ONE socket to the
	// first free port in this range and every later connection reuses it, so any number of
	// concurrent previews still fit - the rest of the range is headroom if 5801 is taken.
	constexpr uint16_t kIcePortRangeBegin = 5801;
	constexpr uint16_t kIcePortRangeEnd = 5809;
}

WebRTCSink::WebRTCSink(std::shared_ptr<Logger> logger, std::string id, WebRTCSinkConfig config)
	: ISink(logger, 1 /* maxSources */, false /* requireJson */, true /* requireFrame */, id)
	, m_Logger(logger)
	, m_Config(config)
{
	if (m_Logger) m_Logger->EnterLog("WebRTCSink constructed, encoder=" + config.encoderName);
	std::lock_guard<std::mutex> lock(m_ConnectionMutex);
	InitializePeerConnection();
}

WebRTCSink::~WebRTCSink()
{
	ShutdownEncoder();
	std::lock_guard<std::mutex> lock(m_ConnectionMutex);
	if (m_PeerConnection) m_PeerConnection->close();
}

// Builds a fresh PeerConnection/Track/SrReporter triple - caller must already hold
// m_ConnectionMutex. Split out of the constructor so CreateOffer() can call it again for every
// new negotiation: a browser tab (re)opening Live Preview creates a brand new RTCPeerConnection
// with its own ICE ufrag/pwd and DTLS fingerprint, and answering it against the OLD, already-
// "stable" native PeerConnection left over from a previous session is rejected outright by
// libdatachannel - confirmed live, on a real Orange Pi with a real camera: stopping and
// restarting Live Preview (or just navigating away from /graph and back) failed every time with
// "WebRTCSink::SetAnswer failed: Unexpected remote answer description in signaling state stable"
// followed by "AddIceCandidate dropped a candidate: Got a remote candidate without ICE
// transport" - the sink's own peer connection was never actually reset between sessions, only
// ever created once in the constructor and reused (and re-answered against) forever after.
void WebRTCSink::InitializePeerConnection()
{
	if (m_PeerConnection) m_PeerConnection->close();

	{
		std::lock_guard<std::mutex> gatheringLock(m_GatheringMutex);
		m_GatheringComplete = false;
	}

	Configuration rtcConfig;
	rtcConfig.portRangeBegin = kIcePortRangeBegin;
	rtcConfig.portRangeEnd = kIcePortRangeEnd;
	rtcConfig.enableIceUdpMux = true;
	m_PeerConnection = std::make_shared<PeerConnection>(rtcConfig);
	m_PeerConnection->onGatheringStateChange([this](PeerConnection::GatheringState state) {
		if (state == PeerConnection::GatheringState::Complete) {
			std::lock_guard<std::mutex> lock(m_GatheringMutex);
			m_GatheringComplete = true;
			m_GatheringCv.notify_all();
		}
	});

	Description::Video video("video", Description::Direction::SendOnly);
	video.addH264Codec(H264_PAYLOAD_TYPE);
	video.addSSRC(kSsrcValue, "lumen-video");
	m_Track = m_PeerConnection->addTrack(video);

	auto rtpConfig = std::make_shared<RtpPacketizationConfig>(kSsrcValue, "lumen-video", H264_PAYLOAD_TYPE, H264RtpPacketizer::ClockRate);
	auto packetizer = std::make_shared<H264RtpPacketizer>(NalUnit::Separator::StartSequence, rtpConfig);
	m_SrReporter = std::make_shared<RtcpSrReporter>(rtpConfig);
	packetizer->addToChain(m_SrReporter);
	packetizer->addToChain(std::make_shared<RtcpNackResponder>());
	m_Track->setMediaHandler(packetizer);
}

std::string WebRTCSink::CreateOffer()
{
	std::shared_ptr<PeerConnection> pc;
	{
		std::lock_guard<std::mutex> lock(m_ConnectionMutex);
		InitializePeerConnection();
		pc = m_PeerConnection;
	}

	pc->setLocalDescription();

	{
		std::unique_lock<std::mutex> lock(m_GatheringMutex);
		if (!m_GatheringCv.wait_for(lock, std::chrono::seconds(10), [this] { return m_GatheringComplete; })) {
			throw std::runtime_error("WebRTCSink::CreateOffer: ICE gathering did not complete within 10s");
		}
	}

	auto description = pc->localDescription();
	if (!description.has_value()) {
		throw std::runtime_error("WebRTCSink::CreateOffer: no local description after gathering completed");
	}
	return std::string(description.value());
}

void WebRTCSink::SetAnswer(const std::string& sdp)
{
	std::shared_ptr<PeerConnection> pc;
	{
		std::lock_guard<std::mutex> lock(m_ConnectionMutex);
		pc = m_PeerConnection;
	}
	try {
		pc->setRemoteDescription(Description(sdp, Description::Type::Answer));
	} catch (const std::exception& e) {
		// Deliberately swallowed, not rethrown: whether a C++ exception thrown here gets
		// marshaled into a well-behaved C# exception at the SWIG/P-Invoke boundary depends on
		// swig.i actually wrapping this call with an %exception typemap, and a neighboring
		// method on this exact class (AddIceCandidate) was confirmed to have no such wrapper -
		// an uncaught exception there crossed the boundary and terminated the whole server
        // process. Not worth gambling on this one being wired correctly: a bad/stale answer just
		// means this connection attempt fails, which the browser side already notices via
		// connectionstatechange/timeout without needing a thrown exception here.
		if (m_Logger) m_Logger->EnterLog(::LogLevel::Error, std::string("WebRTCSink::SetAnswer failed: ") + e.what());
	}
}

void WebRTCSink::AddIceCandidate(const std::string& candidate, const std::string& mid)
{
	std::shared_ptr<PeerConnection> pc;
	{
		std::lock_guard<std::mutex> lock(m_ConnectionMutex);
		pc = m_PeerConnection;
	}
	try {
		pc->addRemoteCandidate(Candidate(candidate, mid));
	} catch (const std::exception& e) {
		// Same reasoning as SetAnswer above - libdatachannel throws std::logic_error if a
		// candidate arrives before the remote description is set (a real race: browsers start
		// firing onicecandidate as soon as setLocalDescription is called, which can beat this
		// sink's /webrtcSink/answer request to the server). A dropped candidate is harmless -
		// ICE negotiation tolerates missing candidates - so this is swallowed rather than
		// rethrown, unlike SetAnswer where the caller genuinely needs to know the answer failed.
		if (m_Logger) m_Logger->EnterLog(::LogLevel::Warning, std::string("WebRTCSink::AddIceCandidate dropped a candidate: ") + e.what());
	}
}

bool WebRTCSink::IsConnected() const
{
	std::lock_guard<std::mutex> lock(m_ConnectionMutex);
	return m_PeerConnection && m_PeerConnection->state() == PeerConnection::State::Connected;
}

std::string WebRTCSink::GetConnectionStatus() const
{
	std::shared_ptr<PeerConnection> pc;
	{
		std::lock_guard<std::mutex> lock(m_ConnectionMutex);
		pc = m_PeerConnection;
	}
	bool gatheringComplete;
	{
		std::lock_guard<std::mutex> gLock(m_GatheringMutex);
		gatheringComplete = m_GatheringComplete;
	}
	nlohmann::json status{
		{"connected", pc && pc->state() == PeerConnection::State::Connected},
		{"iceState", static_cast<int>(pc ? pc->iceState() : PeerConnection::IceState::Closed)},
		{"gatheringComplete", gatheringComplete},
	};
	return status.dump();
}

bool WebRTCSink::EnsureEncoderInitialized(int width, int height)
{
	if (m_CodecContext && m_EncoderWidth == width && m_EncoderHeight == height) {
		return true;
	}
	ShutdownEncoder();

	const AVCodec* codec = avcodec_find_encoder_by_name(m_Config.encoderName.c_str());
	if (!codec) {
		if (m_Logger) m_Logger->EnterLog(::LogLevel::Error, "WebRTCSink: encoder not found: " + m_Config.encoderName);
		return false;
	}

	m_CodecContext = avcodec_alloc_context3(codec);
	m_CodecContext->width = width;
	m_CodecContext->height = height;
	m_CodecContext->time_base = AVRational{ 1, m_Config.fps };
	m_CodecContext->framerate = AVRational{ m_Config.fps, 1 };
	// NV12 when RGA is compiled in - it's the format RgaColorConverter actually produces (and
	// the RK3588 VPU's own native/preferred format for h264_rkmpp regardless), avoiding a
	// pointless extra conversion on top of what RGA already did. YUV420P everywhere else
	// (unchanged from before this optimization) - sws_scale's existing fallback path is left
	// completely untouched on any build that doesn't have RGA hardware to offload to.
#ifdef LUMEN_WITH_RGA
	const AVPixelFormat convertedPixFmt = AV_PIX_FMT_NV12;
#else
	const AVPixelFormat convertedPixFmt = AV_PIX_FMT_YUV420P;
#endif
	m_CodecContext->pix_fmt = convertedPixFmt;
	m_CodecContext->bit_rate = static_cast<int64_t>(m_Config.bitrateKbps) * 1000;
	m_CodecContext->gop_size = m_Config.fps * 2;
	m_CodecContext->max_b_frames = 0; // zero-latency streaming, not file encoding
	av_opt_set(m_CodecContext->priv_data, "preset", "ultrafast", 0);
	av_opt_set(m_CodecContext->priv_data, "tune", "zerolatency", 0);

	if (avcodec_open2(m_CodecContext, codec, nullptr) < 0) {
		if (m_Logger) m_Logger->EnterLog(::LogLevel::Error, "WebRTCSink: avcodec_open2 failed for " + m_Config.encoderName);
		avcodec_free_context(&m_CodecContext);
		return false;
	}

	// always set up, even when RGA is compiled in - the sws_scale path is the runtime fallback
	// whenever RgaColorConverter::ConvertBgrToNv12 fails (busy/absent hardware, unsupported
	// size), not just a build-time alternative - see EncodeAndSend.
	m_SwsContext = sws_getContext(width, height, AV_PIX_FMT_BGR24, width, height, convertedPixFmt,
		SWS_BILINEAR, nullptr, nullptr, nullptr);

	m_YuvFrame = av_frame_alloc();
	m_YuvFrame->format = convertedPixFmt;
	m_YuvFrame->width = width;
	m_YuvFrame->height = height;
	av_frame_get_buffer(m_YuvFrame, 32);

	m_EncoderWidth = width;
	m_EncoderHeight = height;
	m_FrameCounter = 0;
	return true;
}

void WebRTCSink::ShutdownEncoder()
{
	if (m_YuvFrame) av_frame_free(&m_YuvFrame);
	if (m_SwsContext) { sws_freeContext(m_SwsContext); m_SwsContext = nullptr; }
	if (m_CodecContext) avcodec_free_context(&m_CodecContext);
	m_EncoderWidth = m_EncoderHeight = 0;
}

void WebRTCSink::EncodeAndSend(const cv::Mat& bgrFrame)
{
	std::shared_ptr<rtc::Track> track;
	{
		std::lock_guard<std::mutex> lock(m_ConnectionMutex);
		track = m_Track;
	}
	if (!track || !track->isOpen()) return;
	if (!EnsureEncoderInitialized(bgrFrame.cols, bgrFrame.rows)) return;

	// RGA first (hardware, near-zero CPU cost); sws_scale (software) is the fallback whenever
	// it's not compiled in, or the hardware call itself fails at runtime (busy/absent RGA,
	// unsupported size) - not just a build-time either/or, matching how ApriltagDetector falls
	// back from Vulkan to CPU on its own hardware-path failure rather than dropping the frame.
	bool converted = false;
#ifdef LUMEN_WITH_RGA
	converted = m_RgaConverter.ConvertBgrToNv12(bgrFrame, m_YuvFrame);
	if (!converted && m_Logger && !m_RgaConversionFailureLogged) {
		m_Logger->EnterLog(::LogLevel::Warning, "WebRTCSink: RGA colour conversion failed, falling back to sws_scale (logged once)");
		m_RgaConversionFailureLogged = true;
	}
#endif
	if (!converted) {
		const uint8_t* srcSlices[1] = { bgrFrame.data };
		int srcStride[1] = { static_cast<int>(bgrFrame.step) };
		sws_scale(m_SwsContext, srcSlices, srcStride, 0, bgrFrame.rows, m_YuvFrame->data, m_YuvFrame->linesize);
	}
	m_YuvFrame->pts = m_FrameCounter++;

	if (avcodec_send_frame(m_CodecContext, m_YuvFrame) < 0) return;

	AVPacket* packet = av_packet_alloc();
	while (avcodec_receive_packet(m_CodecContext, packet) == 0) {
		binary sample(reinterpret_cast<byte*>(packet->data), reinterpret_cast<byte*>(packet->data) + packet->size);
		double elapsedSeconds = static_cast<double>(m_FrameCounter) / m_Config.fps;
		try {
			track->sendFrame(sample, std::chrono::duration<double>(elapsedSeconds));
		} catch (const std::exception& e) {
			if (m_Logger) m_Logger->EnterLog(::LogLevel::Warning, std::string("WebRTCSink: sendFrame failed: ") + e.what());
		}
		av_packet_unref(packet);
	}
	av_packet_free(&packet);
}

void WebRTCSink::Process(const std::vector<SourceResult>& results)
{
	for (const SourceResult& result : results) {
		if (result.frame.has_value() && !result.frame.value().empty()) {
			EncodeAndSend(result.frame->AsBgr());
		}
	}
}

#endif // LUMEN_WITH_WEBRTC
