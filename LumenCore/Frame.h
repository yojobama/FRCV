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
	// Same as above, plus the pool-owner handle FramePool::Acquire returned alongside `mat` (see
	// FramePool.h) - kept alive here for as long as this Frame (and every copy sharing its
	// Storage) is, so the buffer isn't recycled out from under a Frame still using it. Pass
	// nullptr (or use the other constructors) for a normally-allocated Mat with no pool
	// involvement - not every producer needs to use the pool for this to work correctly.
	Frame(cv::Mat mat, FrameFormat format, std::shared_ptr<void> poolOwner);

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

	// Returns a NEW Frame wrapping the SAME BGR view AsBgr() would (no extra conversion/copy -
	// still computed at most once, cached, exactly like AsBgr() alone), but correctly carrying
	// forward whatever FramePool ownership that view's buffer has. Passing AsBgr()'s raw cv::Mat
	// through a bare Frame/SourceResult constructor instead would silently drop that tracking -
	// a real use-after-recycle risk once the buffer's original Frame (this one) goes out of
	// scope and nothing else is keeping its pool owner alive. Use this, not AsBgr(), at any call
	// site that needs to publish a view as a NEW SourceResult/Frame rather than just read pixels
	// from it locally - ApriltagDetector's driver-mode passthrough is the first such site.
	Frame AsBgrFrame() const;

private:
	struct Storage {
		cv::Mat mat;
		FrameFormat format = FrameFormat::BGR24;
		std::mutex viewMutex;
		std::optional<cv::Mat> grayView;
		std::optional<cv::Mat> bgrView;
		// non-null when `mat` (or, independently, grayView/bgrView) is backed by a FramePool
		// buffer rather than a normally-allocated one - see the pool-taking constructor's comment.
		// mat and the two lazy views can each independently be pool-backed or not; one owner per
		// buffer, matching them up by field name below.
		std::shared_ptr<void> matPoolOwner;
		std::shared_ptr<void> grayPoolOwner;
		std::shared_ptr<void> bgrPoolOwner;
	};
	std::shared_ptr<Storage> m_Storage;
};
