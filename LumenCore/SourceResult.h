#pragma once

#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <memory>
#include <cstdint>
#include "Frame.h"

class SourceResult
{
public:
	SourceResult();
	// Frame's own constructor is implicit from cv::Mat (assumes BGR24), so a bare cv::Mat argument
	// already converts to std::optional<Frame> in one user-defined conversion - every existing
	// producer (ImageFileSource, VideoFileSource, ApriltagDetector, ObjectDetectionSink,
	// StereoDepthNode, DepthFusionNode, OpenCvCameraBackend) keeps compiling unchanged through
	// this single overload. A parallel std::optional<cv::Mat> overload was tried and removed: cv::Mat
	// is reachable via one user-defined conversion to EITHER optional<cv::Mat> or optional<Frame>,
	// so keeping both made every such call ambiguous.
	SourceResult(std::optional<nlohmann::json> json, std::optional<Frame> frame);
	// same as above, plus an explicit capture timestamp - use this overload when the producer
	// knows the real moment the frame was captured (e.g. CameraFrameSource, immediately after
	// cv::VideoCapture::read() returns) rather than letting ISource::SetLatestResult's
	// publish-time fallback stand in for it. Needed by anything that must pair frames from two
	// independent sources (StereoCalibrator/StereoDepthNode) - publish time alone can't tell two
	// free-running cameras' frames apart from ones that are actually far apart in time.
	SourceResult(std::optional<nlohmann::json> json, std::optional<Frame> frame, uint64_t captureTimeUs);
	std::optional<nlohmann::json> json;
	std::optional<Frame> frame;

	// which bound source this result came from, filled in by ISink::ProcessingThreadLoop
	// (not by the producing ISource itself, which has no reason to know its own ID at
	// construction time inside Process()). Needed by any sink bound to more than one source
	// that must know which source a given result belongs to - e.g. NetworkTablesSink publishing
	// each bound detector into its own subtable.
	std::string sourceId;

	// filled in by ISource::SetLatestResult, not by the producing subclass - see its comment for
	// why m_FrameCount already has to live there. Monotonic per-source sequence number, starting
	// at 1 for the first published result.
	uint64_t frameNumber = 0;

	// wall-clock microseconds (since epoch) the frame was actually captured, and the moment
	// SetLatestResult published it. A producer that constructs a SourceResult without an
	// explicit captureTimeUs (the two-argument constructor) gets producedTimeUs used for both -
	// an honest "we don't know any better" fallback, not a claim of precise capture timing.
	uint64_t captureTimeUs = 0;
	uint64_t producedTimeUs = 0;

	// current wall-clock time in microseconds since epoch - the single clock every timestamp in
	// this class is drawn from, so captureTimeUs/producedTimeUs from different sources are
	// directly comparable (e.g. StereoCalibrator/StereoDepthNode's skew gate).
	static uint64_t NowUs();
};
