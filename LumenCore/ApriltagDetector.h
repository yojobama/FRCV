#pragma once
#include "ISink.h"
#include "ISource.h"
#include "IApriltagBackend.h"
#include "AprilTagFieldLayout.h"
#include "CameraCalibrationResult.h"
#include <apriltag/apriltag_pose.h>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d.hpp> // cv::undistortPoints - not pulled in by <opencv2/opencv.hpp> alone
#include <memory>

class Logger;

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
	ApriltagBackendKind GetBackendKind() const { return m_ActiveBackendKind; }

	// lets a caller rebuild an equivalent detector (e.g. to switch backend on an existing sink
	// without losing its tag size/calibration) without needing its own separate tracking of
	// what this detector was originally constructed with - the webui's Inspector "Backend"
	// control on a plain-created ApriltagSink (not one made via a Pipeline Profile) needs
	// exactly this.
	double GetTagSize() const { return m_DetectionInfo.tagsize; }
	CameraCalibrationResult GetCalibration() const;

	// ROADMAP.md Phase 7 (driver mode): when true, Process() skips the actual detection call and
	// NT4 publish entirely and just republishes the raw camera frame - matching PhotonVision's
	// own driver-mode semantics (the driver station still sees live video, but the coprocessor
	// stops spending CPU/NPU time on vision processing while the driver is doing manual scoring/
	// climb/whatever doesn't need tag tracking). Downstream bindings (WebRTCSink in particular)
	// are untouched, so this needs no rewiring - the node graph topology stays exactly as bound.
	void SetDriverMode(bool enabled) { m_DriverMode = enabled; }
	bool GetDriverMode() const { return m_DriverMode; }

	// ROADMAP.md Phase 7 (multi-tag PnP): loads a WPILib-format AprilTagFieldLayout JSON file
	// (the same one a robot program's own WPILib code already loads). Once set, Process()
	// combines every currently-visible tag with a known field pose into one solvePnP call,
	// publishing a single field-relative camera pose alongside the existing per-tag detections -
	// more robust than any one tag's own single-tag estimate, especially at range/oblique angle.
	// Returns false (and leaves multi-tag PnP disabled) on any load failure.
	bool LoadFieldLayout(const std::string& jsonPath) { return m_FieldLayout.LoadFromFile(jsonPath); }
	size_t GetFieldLayoutTagCount() const { return m_FieldLayout.size(); }

	// The actual multi-tag PnP solve, factored out as a public static method so it's directly
	// unit-testable against synthetic correspondences (rendering real AprilTag bitmaps through
	// the full detector pipeline just to exercise this math would be a much heavier test for no
	// more real coverage of the part that's actually at risk of a bug: the PnP/inversion math
	// itself, not tag detection, which already has its own coverage). Returns a null json if
	// tagCount < 2 or solvePnP itself fails; see ApriltagDetector.cpp for the full field-to-
	// camera convention this returns.
	static nlohmann::json SolveMultiTagPnP(
		const std::vector<cv::Point3d>& objectPoints, const std::vector<cv::Point2d>& imagePoints,
		const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, int tagCount);
private:
	void Process(std::vector<SourceResult> results) override;

	std::unique_ptr<IApriltagBackend> m_Backend;
	ApriltagBackendKind m_ActiveBackendKind = APRILTAG_BACKEND_CPU;

	std::shared_ptr<Logger> m_Logger;
	apriltag_detection_info_t m_DetectionInfo;
	// the CameraCalibrationResult this was actually constructed with, kept verbatim (not just
	// derived back out of m_CameraMatrix/m_DistCoeffs, which lose rms/imageWidth/imageHeight) so
	// GetCalibration() can hand it back unchanged to whoever needs to rebuild an equivalent
	// detector (see GetCalibration's own comment).
	CameraCalibrationResult m_OriginalCalibration;

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

	bool m_DriverMode = false;

	AprilTagFieldLayout m_FieldLayout;
};
