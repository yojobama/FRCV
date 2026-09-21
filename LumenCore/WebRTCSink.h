#pragma once
#ifdef LUMEN_WITH_WEBRTC

#include "ISink.h"
#include <rtc/rtc.hpp>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

struct WebRTCSinkConfig {
	int bitrateKbps = 4000;
	int fps = 30;
	// "libx264" everywhere by default; the Orange Pi build can pass "h264_rkmpp" once that
	// hardware encoder is confirmed available there (see IMPLEMENTATION_PLAN.md phase 6) -
	// exposed as a plain string so Manager doesn't need an encoder-name enum for one setting
	std::string encoderName = "libx264";
};

// Terminal sink: binds to any single frame-producing node (raw camera, or a detector's
// annotated output - "different stages" from the plan falls out for free, since each is just
// another ISource) and encodes+streams it over WebRTC. Signalling (offer/answer/ICE) is
// exposed through plain string methods rather than libdatachannel's C++ types, matching
// NetworkTablesSink's pattern for the same reason: those types must never reach swig.i.
//
// Uses non-trickle ICE: CreateOffer() blocks (bounded by a timeout) until this peer's own
// candidate gathering completes, then returns one complete SDP with every local candidate
// already embedded. This trades a little offer latency for a REST-friendly signalling flow -
// no persistent connection is needed on the LumenVision side beyond the C# server's own request
// lifetime, at the cost of not being usable across a p2p link with asymmetric NAT needing
// trickle. Fine for this project's use case (client and LumenVision are on the same LAN).
class WebRTCSink : public ISink {
public:
	WebRTCSink(std::shared_ptr<Logger> logger, std::string id, WebRTCSinkConfig config);
	~WebRTCSink() override;

	// returns the complete local SDP offer once ICE gathering finishes, or throws
	// std::runtime_error on timeout
	std::string CreateOffer();
	void SetAnswer(const std::string& sdp);
	void AddIceCandidate(const std::string& candidate, const std::string& mid);
	bool IsConnected() const;
	std::string GetConnectionStatus() const;

private:
	void Process(std::vector<SourceResult> results) override;

	bool EnsureEncoderInitialized(int width, int height);
	void EncodeAndSend(const cv::Mat& bgrFrame);
	void ShutdownEncoder();

	std::shared_ptr<Logger> m_Logger;
	WebRTCSinkConfig m_Config;

	std::shared_ptr<rtc::PeerConnection> m_PeerConnection;
	std::shared_ptr<rtc::Track> m_Track;
	std::shared_ptr<rtc::RtcpSrReporter> m_SrReporter;

	std::mutex m_GatheringMutex;
	std::condition_variable m_GatheringCv;
	bool m_GatheringComplete = false;

	AVCodecContext* m_CodecContext = nullptr;
	SwsContext* m_SwsContext = nullptr;
	AVFrame* m_YuvFrame = nullptr;
	int m_EncoderWidth = 0;
	int m_EncoderHeight = 0;
	int64_t m_FrameCounter = 0;
};

#endif // LUMEN_WITH_WEBRTC
