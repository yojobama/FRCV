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

// The runtime-tunable detector knobs, shared by both backends so a sink can be rebuilt (backend
// switch, frame-size change) with exactly the same settings.
//   nthreads     <= 0: the backend's own default (CPU: apriltag's 1; Vulkan: hardware_concurrency)
//   quadDecimate <= 0: the backend's own default (2 for both). Vulkan only supports integers and
//                      needs the frame to be divisible by it - see VkApriltagBackend.
//   refineEdges:       libapriltag's refine_edges (gradient-based corner refinement). Default true,
//                      libapriltag's own default - recommended specifically to compensate for
//                      decimation's coarser quads, and cheap relative to detection.
struct ApriltagTuning {
	int nthreads = 0;
	float quadDecimate = 0.0f;
	bool refineEdges = true;
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

	// Runtime tuning knobs - reported back (not just "what was requested") so a caller can see
	// what's actually in effect, same "requested vs actual" honesty IApriltagBackend already
	// has via Name()/GetBackendName() for the CPU<->Vulkan fallback. CpuApriltagBackend backs all
	// three directly; VkApriltagBackend's GetThreads() maps to its own cpu_threads (the CPU-tail
	// worker pool, the genuinely analogous knob), and its decimation is an integer baked into the
	// GPU pipeline at construction, so GetQuadDecimate() reports the value actually in use (which
	// can differ from the request - see VkApriltagBackend.cpp). GetQuadDecimateSupported() stays
	// so a caller (the webui's Inspector) can tell whether a control would do anything.
	virtual int GetThreads() const = 0;
	virtual float GetQuadDecimate() const = 0;
	virtual bool GetQuadDecimateSupported() const = 0;
	virtual bool GetRefineEdges() const = 0;
};
