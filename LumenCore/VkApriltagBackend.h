#pragma once
#ifdef LUMEN_WITH_VULKAN_APRILTAG

#include "IApriltagBackend.h"
#include <vkapriltag/TagDecoder.h>
#include <vkapriltag/gpu/GpuDetector.h>
#include <vkapriltag/gpu/QuadDecode.h>
#include <vkapriltag/vk/Context.h>
#include <memory>

// GPU (Vulkan compute) AprilTag detection via the vkapriltag submodule (third_party/vkapriltag).
//
// Critical constraint, confirmed by actually building and linking both: vkapriltag statically
// links a *patched* fork of AprilRobotics/apriltag (same v3.4.5, plus two extra exported
// symbols: quad_decode_index, reconcile_detections - see
// third_party/vkapriltag/apriltags_vulkan/cmake/patches/apriltag-expose-decode-steps.patch).
// That patched build's shared library carries the SAME SONAME (libapriltag.so.3) as a vanilla
// build or apt's package, so there can only be one apriltag.so.3 anywhere this links - it must
// be the patched one (install-deps.sh's build_apriltag() builds and installs exactly that, and
// purges apt's package rather than let it collide). CpuApriltagBackend links the identical
// library, just never touches the two extra symbols - the patch changes no existing behavior,
// so this costs the CPU backend nothing.
//
// TagDecoder::Decode() produces the same zarray_t* of apriltag_detection_t* that
// apriltag_detector_detect() does - it stops at 2D detection (no pose), matching this project's
// design of keeping pose estimation, JSON emission and frame annotation shared in
// ApriltagDetector regardless of which IApriltagBackend produced the detections.
class VkApriltagBackend : public IApriltagBackend {
public:
	// throws (via vk::Context's constructor / CheckVk) if no usable Vulkan compute device is
	// found - callers should catch this and fall back to CpuApriltagBackend, matching the
	// runtime capability probe called for in the implementation plan (phase 5, item 5); this
	// class makes no attempt to be silently CPU-safe itself.
	// cpuThreads <= 0 means "leave DetectorConfig's own default (0 = hardware_concurrency())
	// alone" - see .cpp's own comment on why this project's caller picks a different default.
	explicit VkApriltagBackend(int frameWidth, int frameHeight, int cpuThreads = 0);
	~VkApriltagBackend() override;

	zarray_t* Detect(const cv::Mat& grayFrame) override;
	void ReleaseResult(zarray_t* detections) override; // no-op: TagDecoder owns its zarray_t
	std::string Name() const override { return "Vulkan (vkapriltag)"; }

	// QuadDecode's own cpu_threads getter (the CPU-tail worker pool - the one part of this
	// pipeline actually running on ordinary CPU threads, decimation/quad-selection is all GPU
	// compute) - the genuinely analogous knob to CpuApriltagBackend's nthreads.
	int GetThreads() const override { return static_cast<int>(m_QuadDecode->threads()); }
	// Fixed at 2x in GpuDetector's own pipeline (see this class's .cpp comment) - not a
	// DetectorConfig field, so there is nothing to read back per-instance; reported as a
	// constant purely so a caller can display what's actually happening.
	float GetQuadDecimate() const override { return 2.0f; }
	bool GetQuadDecimateSupported() const override { return false; }

private:
	apriltag_detector_t* m_Detector;
	apriltag_family_t* m_Family;

	std::unique_ptr<apriltag_vulkan::vk::Context> m_Context;
	std::unique_ptr<apriltag_vulkan::GpuDetector> m_GpuDetector;
	std::unique_ptr<apriltag_vulkan::QuadDecode> m_QuadDecode;
	std::unique_ptr<apriltag_vulkan::TagDecoder> m_TagDecoder;
};

#endif // LUMEN_WITH_VULKAN_APRILTAG
