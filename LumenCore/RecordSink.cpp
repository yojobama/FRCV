#ifdef LUMEN_WITH_RECORD
#include "RecordSink.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <algorithm>
#include <cstdio>

namespace {
	// zero-padded so a plain filename sort IS chronological order - ListSegments()/
	// EnforceRetention() both rely on this rather than re-parsing a timestamp out of the name.
	std::string SegmentBaseName(int index)
	{
		char buf[32];
		std::snprintf(buf, sizeof(buf), "segment_%06d", index);
		return buf;
	}

	// basic sanitization for a filename that ultimately reaches std::filesystem::remove() -
	// Server/RecordSinkController.cs must do its own equivalent check before ever calling
	// DeleteSegment (a caller-supplied string reaching this far shouldn't be trusted twice, but
	// this is the one function that actually deletes a file, so it checks too).
	bool IsSafeSegmentFilename(const std::string& filename)
	{
		if (filename.empty()) return false;
		if (filename.find('/') != std::string::npos) return false;
		if (filename.find('\\') != std::string::npos) return false;
		if (filename.find("..") != std::string::npos) return false;
		return true;
	}
}

RecordSink::RecordSink(std::shared_ptr<Logger> logger, std::string id, RecordSinkConfig config)
	: ISink(logger, 1 /* maxSources */, false /* requireJson */, true /* requireFrame */, id)
	, m_Logger(logger)
	, m_Config(config)
{
	if (m_Logger) m_Logger->EnterLog("RecordSink constructed, dstFolder=" + config.dstFolder + ", encoder=" + config.encoderName);
	std::error_code ec;
	std::filesystem::create_directories(config.dstFolder, ec);
	if (ec && m_Logger) {
		m_Logger->EnterLog(LogLevel::Error, "RecordSink: failed to create dstFolder " + config.dstFolder + ": " + ec.message());
	}
}

RecordSink::~RecordSink()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	CloseCurrentSegment();
	ShutdownEncoder();
}

std::vector<std::string> RecordSink::ListSegments() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	std::vector<std::string> names;
	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(m_Config.dstFolder, ec)) {
		if (!entry.is_regular_file()) continue;
		if (entry.path().extension() != ".mp4") continue;
		names.push_back(entry.path().filename().string());
	}
	// newest first - SegmentBaseName's zero-padding makes a plain string sort chronological,
	// descending gives newest-first without parsing anything back out of the name.
	std::sort(names.rbegin(), names.rend());
	return names;
}

bool RecordSink::DeleteSegment(const std::string& filename)
{
	if (!IsSafeSegmentFilename(filename)) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Warning, "RecordSink::DeleteSegment refused an unsafe filename: " + filename);
		return false;
	}
	std::lock_guard<std::mutex> lock(m_Mutex);
	std::filesystem::path videoPath = std::filesystem::path(m_Config.dstFolder) / filename;
	std::error_code ec;
	bool removed = std::filesystem::remove(videoPath, ec) && !ec;
	// best-effort - the sidecar not existing (or failing to delete) shouldn't make DeleteSegment
	// itself report failure once the actual video file is gone.
	std::filesystem::path sidecarPath = videoPath;
	sidecarPath.replace_extension(".jsonl");
	std::filesystem::remove(sidecarPath, ec);
	return removed;
}

void RecordSink::EnforceRetention()
{
	if (m_Config.maxFileCount <= 0 && m_Config.maxFolderSizeBytes <= 0) return;

	struct SegmentInfo { std::string filename; uintmax_t size; };
	std::vector<SegmentInfo> segments;
	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(m_Config.dstFolder, ec)) {
		if (!entry.is_regular_file() || entry.path().extension() != ".mp4") continue;
		segments.push_back({ entry.path().filename().string(), entry.file_size(ec) });
	}
	// oldest first, so eviction below removes the oldest segment first - the reverse order
	// ListSegments() itself returns.
	std::sort(segments.begin(), segments.end(), [](const SegmentInfo& a, const SegmentInfo& b) { return a.filename < b.filename; });

	// totalSize covers every segment, including the current one - the folder-size cap is a real
	// disk-usage bound, not just a bound on what's evictable.
	uintmax_t totalSize = 0;
	for (const auto& s : segments) totalSize += s.size;

	// never evict the segment currently being written - it's always the newest by construction
	// (StartNewSegment already advanced m_SegmentIndex before this runs), so everything except
	// the last entry after the ascending sort above is eligible, nothing more.
	size_t evictableCount = segments.empty() ? 0 : segments.size() - 1;

	size_t index = 0;
	while (index < evictableCount &&
		((m_Config.maxFileCount > 0 && static_cast<int>(segments.size() - index) > m_Config.maxFileCount) ||
		 (m_Config.maxFolderSizeBytes > 0 && totalSize > static_cast<uintmax_t>(m_Config.maxFolderSizeBytes)))) {
		const SegmentInfo& oldest = segments[index];
		std::filesystem::path videoPath = std::filesystem::path(m_Config.dstFolder) / oldest.filename;
		std::filesystem::remove(videoPath, ec);
		std::filesystem::path sidecarPath = videoPath;
		sidecarPath.replace_extension(".jsonl");
		std::filesystem::remove(sidecarPath, ec);
		if (m_Logger) m_Logger->EnterLog("RecordSink: retention deleted " + oldest.filename);
		totalSize -= oldest.size;
		index++;
	}
}

bool RecordSink::EnsureEncoderInitialized(int width, int height)
{
	if (m_CodecContext && m_EncoderWidth == width && m_EncoderHeight == height) {
		return true;
	}
	ShutdownEncoder();

	const AVCodec* codec = avcodec_find_encoder_by_name(m_Config.encoderName.c_str());
	if (!codec) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "RecordSink: encoder not found: " + m_Config.encoderName);
		return false;
	}

	m_CodecContext = avcodec_alloc_context3(codec);
	m_CodecContext->width = width;
	m_CodecContext->height = height;
	m_CodecContext->time_base = AVRational{ 1, m_Config.fps };
	m_CodecContext->framerate = AVRational{ m_Config.fps, 1 };
	m_CodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	m_CodecContext->bit_rate = static_cast<int64_t>(m_Config.bitrateKbps) * 1000;
	m_CodecContext->gop_size = m_Config.fps * 2;
	// no B-frames, deliberately: this project's own MP4 muxing has to get DTS/PTS reordering
	// right if it allows them, which is real additional correctness surface for a first pass -
	// see RecordSink.h's own comment. Costs some compression efficiency versus a from-scratch
	// tuned encoder, not correctness.
	m_CodecContext->max_b_frames = 0;
	// MP4 always wants SPS/PPS in the stream's own extradata (the avcC box), not repeated in
	// front of every keyframe the way a raw/RTP stream wants them - WebRTCSink never needs this
	// flag since it never muxes a container at all.
	m_CodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	// "medium" (libx264's own default preset) rather than WebRTCSink's "ultrafast"/"zerolatency" -
	// this is written once and re-watched/edited many times, not a live low-latency stream, so
	// spending more CPU per frame for meaningfully better compression is the right tradeoff here.
	av_opt_set(m_CodecContext->priv_data, "preset", "medium", 0);

	if (avcodec_open2(m_CodecContext, codec, nullptr) < 0) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "RecordSink: avcodec_open2 failed for " + m_Config.encoderName);
		avcodec_free_context(&m_CodecContext);
		return false;
	}

	m_SwsContext = sws_getContext(width, height, AV_PIX_FMT_BGR24, width, height, AV_PIX_FMT_YUV420P,
		SWS_BILINEAR, nullptr, nullptr, nullptr);

	m_YuvFrame = av_frame_alloc();
	m_YuvFrame->format = AV_PIX_FMT_YUV420P;
	m_YuvFrame->width = width;
	m_YuvFrame->height = height;
	av_frame_get_buffer(m_YuvFrame, 32);

	m_EncoderWidth = width;
	m_EncoderHeight = height;
	return true;
}

void RecordSink::ShutdownEncoder()
{
	if (m_YuvFrame) av_frame_free(&m_YuvFrame);
	if (m_SwsContext) { sws_freeContext(m_SwsContext); m_SwsContext = nullptr; }
	if (m_CodecContext) avcodec_free_context(&m_CodecContext);
	m_EncoderWidth = m_EncoderHeight = 0;
}

void RecordSink::StartNewSegment(int width, int height)
{
	CloseCurrentSegment();

	if (!EnsureEncoderInitialized(width, height)) return;

	std::string baseName = SegmentBaseName(m_SegmentIndex++);
	std::filesystem::path videoPath = std::filesystem::path(m_Config.dstFolder) / (baseName + ".mp4");
	std::filesystem::path sidecarPath = std::filesystem::path(m_Config.dstFolder) / (baseName + ".jsonl");

	if (avformat_alloc_output_context2(&m_FormatContext, nullptr, "mp4", videoPath.string().c_str()) < 0 || !m_FormatContext) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "RecordSink: avformat_alloc_output_context2 failed for " + videoPath.string());
		return;
	}

	m_VideoStream = avformat_new_stream(m_FormatContext, nullptr);
	avcodec_parameters_from_context(m_VideoStream->codecpar, m_CodecContext);
	m_VideoStream->time_base = m_CodecContext->time_base;

	if (avio_open(&m_FormatContext->pb, videoPath.string().c_str(), AVIO_FLAG_WRITE) < 0) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "RecordSink: avio_open failed for " + videoPath.string());
		avformat_free_context(m_FormatContext);
		m_FormatContext = nullptr;
		m_VideoStream = nullptr;
		return;
	}

	if (avformat_write_header(m_FormatContext, nullptr) < 0) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "RecordSink: avformat_write_header failed for " + videoPath.string());
		avio_closep(&m_FormatContext->pb);
		avformat_free_context(m_FormatContext);
		m_FormatContext = nullptr;
		m_VideoStream = nullptr;
		return;
	}

	m_TelemetrySidecar.open(sidecarPath, std::ios::out | std::ios::trunc);
	m_FrameCounter = 0;
	m_SegmentStartedAt = std::chrono::steady_clock::now();
	if (m_Logger) m_Logger->EnterLog("RecordSink: started segment " + videoPath.string());

	EnforceRetention();
}

void RecordSink::CloseCurrentSegment()
{
	if (m_FormatContext) {
		av_write_trailer(m_FormatContext);
		avio_closep(&m_FormatContext->pb);
		avformat_free_context(m_FormatContext);
		m_FormatContext = nullptr;
		m_VideoStream = nullptr;
	}
	if (m_TelemetrySidecar.is_open()) {
		m_TelemetrySidecar.close();
	}
}

void RecordSink::EncodeAndWrite(const cv::Mat& bgrFrame, const SourceResult& result)
{
	bool segmentExpired = m_FormatContext &&
		std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_SegmentStartedAt).count() >= m_Config.segmentSeconds;
	if (!m_FormatContext || m_EncoderWidth != bgrFrame.cols || m_EncoderHeight != bgrFrame.rows || segmentExpired) {
		StartNewSegment(bgrFrame.cols, bgrFrame.rows);
	}
	if (!m_FormatContext || !m_CodecContext) return;

	const uint8_t* srcSlices[1] = { bgrFrame.data };
	int srcStride[1] = { static_cast<int>(bgrFrame.step) };
	sws_scale(m_SwsContext, srcSlices, srcStride, 0, bgrFrame.rows, m_YuvFrame->data, m_YuvFrame->linesize);
	m_YuvFrame->pts = m_FrameCounter++;

	if (avcodec_send_frame(m_CodecContext, m_YuvFrame) < 0) return;

	AVPacket* packet = av_packet_alloc();
	while (avcodec_receive_packet(m_CodecContext, packet) == 0) {
		packet->stream_index = m_VideoStream->index;
		av_packet_rescale_ts(packet, m_CodecContext->time_base, m_VideoStream->time_base);
		av_interleaved_write_frame(m_FormatContext, packet);
		av_packet_unref(packet);
	}
	av_packet_free(&packet);

	if (m_TelemetrySidecar.is_open()) {
		nlohmann::json record{
			{"frameNumber", result.frameNumber},
			{"captureTimeUs", result.captureTimeUs},
			{"producedTimeUs", result.producedTimeUs},
			{"json", result.json.has_value() ? result.json.value() : nlohmann::json(nullptr)},
		};
		// flushed every line, not buffered - this sidecar is meant to be tailable/inspectable
		// while a recording is still in progress, and the per-line cost is trivial next to the
		// H.264 encode this already did above.
		m_TelemetrySidecar << record.dump() << "\n";
		m_TelemetrySidecar.flush();
	}
}

void RecordSink::Process(const std::vector<SourceResult>& results)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	for (const SourceResult& result : results) {
		if (result.frame.has_value() && !result.frame.value().empty()) {
			EncodeAndWrite(result.frame->AsBgr(), result);
		}
	}
}

#endif // LUMEN_WITH_RECORD
