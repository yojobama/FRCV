#pragma once
#include "IApriltagBackend.h"
#include <apriltag/tag36h11.h>

// Wraps today's apriltag_detector_detect() call - unchanged behavior, just moved out of
// ApriltagDetector so it sits behind IApriltagBackend next to VkApriltagBackend.
class CpuApriltagBackend : public IApriltagBackend {
public:
	// nthreads/quadDecimate <= 0 means "leave apriltag_detector_create()'s own default alone"
	// (1 thread, no decimation) - see the .cpp's own comment for why this project's caller
	// chooses different defaults instead of hardcoding them here.
	explicit CpuApriltagBackend(int nthreads = 0, float quadDecimate = 0.0f);
	~CpuApriltagBackend() override;

	zarray_t* Detect(const cv::Mat& grayFrame) override;
	void ReleaseResult(zarray_t* detections) override;
	std::string Name() const override { return "CPU (apriltag)"; }

	int GetThreads() const override { return m_Detector->nthreads; }
	float GetQuadDecimate() const override { return m_Detector->quad_decimate; }
	bool GetQuadDecimateSupported() const override { return true; }

private:
	apriltag_detector_t* m_Detector;
	apriltag_family_t* m_Family;
};
