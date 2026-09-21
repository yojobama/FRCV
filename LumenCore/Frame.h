#pragma once
#include <opencv2/opencv.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include "FrameFormat.h"

// A refcounted, format-tagged frame handle. Copying a Frame is a refcount bump (shared_ptr-
// backed), not a pixel copy - required, not stylistic, since SourceResult (which holds one) is
// already copied by value throughout this codebase (ISink.cpp, ISource.cpp, StereoCalibrator's
// pending-pair state).
//
// AsGray()/AsBgr() are lazily computed and cached on first call: for a GRAY8/NV12 source,
// AsGray() is free (the Y plane already IS the grayscale image, no conversion needed); for
// BGR24/RGB24, AsBgr() is free the same way (BGR24 outright, RGB24 needs the actual OpenCV
// convention swap). Converting the OTHER direction costs one real cv::cvtColor call, done once
// and cached rather than repeated on every access - this is what replaces ApriltagDetector.cpp's
// own inline cvtColor(BGR2GRAY) call.
class Frame
{
public:
	Frame() = default;

	// Implicit on purpose: every current producer (ImageFileSource, VideoFileSource,
	// ApriltagDetector, ObjectDetectionSink, StereoDepthNode, DepthFusionNode,
	// OpenCvCameraBackend) constructs a result from a bare cv::Mat today, always BGR - this
	// constructor is what lets every one of them keep compiling unchanged through this
	// migration (see SourceResult's own cv::Mat-taking constructor overloads, which exist for
	// exactly the same reason). Deliberately no implicit conversion the OTHER way (Frame ->
	// cv::Mat): chained user-defined conversions (Frame -> cv::Mat -> cv::_InputArray) are not
	// something the language allows anyway, so every consumption site stays explicit
	// (.AsGray()/.AsBgr()) instead of compiling inconsistently for non-obvious reasons.
	Frame(const cv::Mat& mat);
	Frame(cv::Mat mat, FrameFormat format);

	bool empty() const;
	cv::Size size() const;
	FrameFormat format() const;

	// A zero-copy view onto a rectangular region of this Frame, tagged with the same format -
	// what RoiSource uses to split one side-by-side (or top-bottom) camera's frame into two
	// independent per-eye Frames without a pixel copy, the same way cv::Mat(mat, roi) is zero-copy
	// (the returned Frame's own Storage wraps that ROI view, not a clone of it). AsGray()/AsBgr()
	// on the result still lazily convert+cache exactly as they would on any other Frame - the ROI
	// view is a distinct Frame with its own cache, not aliased to this Frame's.
	Frame Roi(const cv::Rect& roi) const;

	// Views - cheap (a refcount bump on the underlying cv::Mat) when the source format already
	// IS the requested one, one real conversion (cached, thread-safe - a Frame is routinely
	// shared across the producing ISource's thread and every bound ISink's own thread) otherwise.
	const cv::Mat& AsGray() const;
	const cv::Mat& AsBgr() const;

private:
	struct Storage {
		cv::Mat mat;
		FrameFormat format = FrameFormat::BGR24;
		std::mutex viewMutex;
		std::optional<cv::Mat> grayView;
		std::optional<cv::Mat> bgrView;
	};
	std::shared_ptr<Storage> m_Storage;
};
