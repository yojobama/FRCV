#pragma once
#include <apriltag/apriltag.h>
#include <opencv2/opencv.hpp>

// Plain (unscoped) enum, not `enum class`: SWIG wraps a plain C++ enum as a real C# enum but
// falls back to a broken opaque handle for a scoped one - confirmed the hard way with
// YoloVariant (IDetectionBackend.h). Kept here rather than in ApriltagDetector.h so the CPU
// backend doesn't need to include the Vulkan one just to see this selector.
enum ApriltagBackendKind {
	APRILTAG_BACKEND_CPU,
	APRILTAG_BACKEND_VULKAN
};

// Detection-only backend abstraction: both implementations hand back a zarray_t* of
// apriltag_detection_t* using the exact same apriltag library (see VkApriltagBackend.h for why
// that must be true), so ApriltagDetector's pose estimation, JSON emission and frame annotation
// stay identical regardless of which backend produced the detections.
class IApriltagBackend {
public:
	virtual ~IApriltagBackend() = default;

	// returns a zarray_t* of apriltag_detection_t*, owned by the backend and valid only until
	// the next Detect() call or the backend's destruction - same lifetime contract as
	// apriltag_detector_detect() itself, so existing call sites don't need special-casing
	virtual zarray_t* Detect(const cv::Mat& grayFrame) = 0;

	// releases a result previously returned by Detect() - CPU uses apriltag_detections_destroy,
	// Vulkan's TagDecoder owns its own zarray_t internally and this is a no-op for it
	virtual void ReleaseResult(zarray_t* detections) = 0;

	virtual std::string Name() const = 0;
};
