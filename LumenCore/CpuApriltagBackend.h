#pragma once
#include "IApriltagBackend.h"
#include <apriltag/tag36h11.h>

// Wraps today's apriltag_detector_detect() call - unchanged behavior, just moved out of
// ApriltagDetector so it sits behind IApriltagBackend next to VkApriltagBackend.
class CpuApriltagBackend : public IApriltagBackend {
public:
	CpuApriltagBackend();
	~CpuApriltagBackend() override;

	zarray_t* Detect(const cv::Mat& grayFrame) override;
	void ReleaseResult(zarray_t* detections) override;
	std::string Name() const override { return "CPU (apriltag)"; }

private:
	apriltag_detector_t* m_Detector;
	apriltag_family_t* m_Family;
};
