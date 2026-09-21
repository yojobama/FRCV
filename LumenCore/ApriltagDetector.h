#pragma once
#include "ISink.h"
#include "ISource.h"
#include "IApriltagBackend.h"
#include <apriltag/apriltag_pose.h>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d.hpp> // cv::undistortPoints - not pulled in by <opencv2/opencv.hpp> alone
#include <memory>

class Logger;
class CameraCalibrationResult;

class ApriltagDetector : public ISink, public ISource
{
public:
	ApriltagDetector(std::shared_ptr<Logger> logger, std::string id, CameraCalibrationResult calibrationResult,
		double tagSize /* in METERS you bloody Americans */,
		ApriltagBackendKind backendKind = APRILTAG_BACKEND_CPU,
		int frameWidth = 0, int frameHeight = 0 /* only consulted for APRILTAG_BACKEND_VULKAN */);
	~ApriltagDetector();

	// which backend actually ended up running - may differ from what was requested if Vulkan
	// was asked for and no usable device was found (falls back to CPU rather than failing to
	// construct at all; see phase 5 item 5 in the implementation plan)
	std::string GetBackendName() const;
private:
	void Process(std::vector<SourceResult> results) override;

	std::unique_ptr<IApriltagBackend> m_Backend;

	std::shared_ptr<Logger> m_Logger;
	apriltag_detection_info_t m_DetectionInfo;

	// present only when the supplied CameraCalibrationResult had real distortion coefficients;
	// when absent, pose estimation runs on the raw detected corners as before (best-effort,
	// biased by whatever distortion the lens actually has)
	bool m_HasDistortion = false;
	cv::Mat m_CameraMatrix;
	cv::Mat m_DistCoeffs;

	// false when constructed without a real calibration (fx/fy left at their 0.0 default, e.g.
	// via createWithBackend before a calibration is attached). estimate_tag_pose has no way to
	// signal "these intrinsics are degenerate" - given fx=fy=0 it still returns, but pose.R/pose.t
	// come back as garbage/invalid pointers, and dereferencing or matd_destroy-ing them corrupts
	// the heap (confirmed by reproducing standalone: crashes on pose.t->data[0]). Pose estimation
	// must be skipped entirely without valid intrinsics, not merely "best-effort".
	bool m_HasCalibration = false;
};
