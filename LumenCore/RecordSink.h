#pragma once
#ifdef LUMEN_WITH_RECORD

#include "ISink.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

struct RecordSinkConfig {
	// relative to the server's own working directory, matching ImageFileSource/VideoFileSource's
	// own "images/"/"videos/" convention rather than an absolute path - see
	// ImageFileSourceController.cs's own comment on why. Created if it doesn't exist yet.
	std::string dstFolder;
	// "libx264" everywhere by default, same knob WebRTCSinkConfig already exposes (including the
	// Orange Pi's "h264_rkmpp" swap once available there) - see cmake/LumenFFmpeg.cmake's own
	// comment for why this specific choice carries a real GPL-licensing consequence for a file
	// that gets downloaded and redistributed, more directly than WebRTCSink's ephemeral RTP use.
	std::string encoderName = "libx264";
	// higher than WebRTCSink's 4000 default - this is for re-watching/editing footage, not a
	// live-latency-bounded preview, so it's worth spending more bits on quality.
	int bitrateKbps = 8000;
	int fps = 30;
	// a new segment starts automatically once the current one has been open this long - bounds
	// how much footage a crash/power-loss mid-segment can lose (only the CURRENT segment is ever
	// at risk; every prior one already has a clean avformat_write_header/av_write_trailer pair)
	// and avoids one unplayable-until-finalized multi-hour file.
	int segmentSeconds = 300;
	// 0 = unbounded (default - see RecordSink.h's own top comment on why this doesn't default to
	// silently deleting a team's own footage). Once set, EnforceRetention() deletes the OLDEST
	// segment (by filename, which sorts chronologically - see StartNewSegment) first.
	int64_t maxFolderSizeBytes = 0;
	int maxFileCount = 0;
};

// Terminal sink: binds to any single frame-producing node (raw camera, or a detector's annotated
// output, same dual-role binding every other terminal sink already gets for free) and writes it
// to segmented MP4 files, plus a JSON-Lines telemetry sidecar per segment (one line per frame:
// frameNumber/captureTimeUs/producedTimeUs/json - the exact same SourceResult fields
// NetworkTablesSink already publishes from, see SourceResult.h) so a team can correlate an exact
// video timestamp to an exact detection result after a match.
//
// The one genuinely new piece of ground this class covers versus WebRTCSink: WebRTCSink already
// hand-rolls H.264 encoding via raw libavcodec/libswscale, but feeds the raw NAL units straight
// into libdatachannel's RTP packetizer - it never touches libavformat's muxing API at all. This
// class reuses WebRTCSink's own encoder-setup shape (EnsureEncoderInitialized/ShutdownEncoder
// mirror it closely on purpose) but adds the actual container muxing
// (avformat_alloc_output_context2/avio_open/avformat_write_header/av_interleaved_write_frame/
// av_write_trailer) that produces a real, playable file - this is the first LumenCore code to do
// so.
class RecordSink : public ISink {
public:
	RecordSink(std::shared_ptr<Logger> logger, std::string id, RecordSinkConfig config);
	~RecordSink() override;

	// filenames only (not full paths), newest first - matches MjpegSink's own "plain std::string
	// crosses the SWIG boundary cleanly" reasoning; Server/RecordSinkController.cs resolves these
	// against this sink's own dstFolder, never a caller-supplied path.
	std::vector<std::string> ListSegments() const;
	// true if filename existed and was removed. Refuses (returns false) if filename resolves
	// outside dstFolder once normalized - same path-traversal concern the download/delete REST
	// endpoints must also guard, checked here too since this is the one function actually calling
	// remove() rather than trusting the C# layer's own check alone.
	bool DeleteSegment(const std::string& filename);

private:
	void Process(const std::vector<SourceResult>& results) override;
	// ISink::Toggle(false), once the processing thread has already stopped - finalizes whatever
	// segment was still open (real trailer written, sidecar closed) so a user stopping a
	// recording gets back a genuinely playable file immediately. See ISink.h's own comment.
	void OnStopped() override;

	void StartNewSegment(int width, int height); // caller must already hold m_Mutex
	void CloseCurrentSegment();                  // caller must already hold m_Mutex
	void EnforceRetention();                     // caller must already hold m_Mutex

	bool EnsureEncoderInitialized(int width, int height); // caller must already hold m_Mutex
	void ShutdownEncoder();                                // caller must already hold m_Mutex
	void EncodeAndWrite(const cv::Mat& bgrFrame, const SourceResult& result); // caller must already hold m_Mutex

	std::shared_ptr<Logger> m_Logger;
	RecordSinkConfig m_Config;

	// guards every member below - Process() runs on ISink's own background thread; ListSegments/
	// DeleteSegment can be called from an HTTP request thread at the same time.
	mutable std::mutex m_Mutex;

	AVFormatContext* m_FormatContext = nullptr;
	AVStream* m_VideoStream = nullptr;
	AVCodecContext* m_CodecContext = nullptr;
	SwsContext* m_SwsContext = nullptr;
	AVFrame* m_YuvFrame = nullptr;
	int m_EncoderWidth = 0;
	int m_EncoderHeight = 0;
	int64_t m_FrameCounter = 0; // resets to 0 at the start of each segment - AVStream pts is per-file

	std::ofstream m_TelemetrySidecar;
	std::chrono::steady_clock::time_point m_SegmentStartedAt;
	int m_SegmentIndex = 0; // monotonically increasing across this sink's whole lifetime, never reused
};

#endif // LUMEN_WITH_RECORD
