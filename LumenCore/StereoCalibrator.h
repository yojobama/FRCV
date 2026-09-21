#pragma once
#include "ISource.h"
#include "ISink.h"
#include "IStereoRoleReceiver.h"
#include "CameraCalibrationResult.h"
#include "StereoCalibrationResult.h"
#include "CalibrationBoardType.h"
#include "StereoPairer.h"

#include <opencv2/calib3d.hpp>
#include <mutex>
#include <optional>

// Checkerboard only for now - see STEREO_IMPLEMENTATION_PLAN.md ss10.2: a ChArUco board can
// detect a different corner subset in each eye, which cv::stereoCalibrate can't consume without
// first intersecting both eyes' detections by corner ID and rebuilding the object-point list
// from that intersection. Deliberately deferred rather than half-done; BOARD_CHARUCO is accepted
// here for API symmetry with CalibrationBoardConfig/CameraCalibrator but throws if actually used.
struct StereoCalibrationBoardConfig {
	CalibrationBoardType type = BOARD_CHECKERBOARD;
	int rows = 6;
	int cols = 9;
	float squareSizeMeters = 0.025f;
};

// ISource+ISink, maxSources=2 (left/right cameras). Mirrors CameraCalibrator's shape and the
// same detection logic (findChessboardCorners + cornerSubPix), applied once per eye per paired
// frame rather than once per frame - see STEREO_IMPLEMENTATION_PLAN.md P1/P2 for why pairing and
// left/right roles need to be handled explicitly rather than reusing ISink's generic bind/
// process path unchanged.
class StereoCalibrator : public ISink, public ISource, public IStereoRoleReceiver
{
public:
	StereoCalibrator(std::shared_ptr<Logger> logger, std::string id,
		StereoCalibrationBoardConfig boardConfig = StereoCalibrationBoardConfig(),
		int64_t maxSkewUs = 33000 /* ~1 frame at 30fps */);

	// IStereoRoleReceiver
	void SetStereoRoles(const std::string& leftSourceId, const std::string& rightSourceId) override;

	// optional: seed per-eye intrinsics from two already-run CameraCalibrator nodes, so
	// RunCalibration() can pass cv::CALIB_FIX_INTRINSIC instead of solving for intrinsics AND
	// extrinsics at once from (typically fewer) stereo-only snapshots - numerically more stable,
	// and reuses calibration work an operator may have already done. Optional: leave unset to
	// solve full intrinsics+extrinsics from the stereo snapshots alone.
	void SetPriorIntrinsics(const CameraCalibrationResult& left, const CameraCalibrationResult& right);

	// saves the most recently matched (both-eyes-found, within-skew) checkerboard detection.
	// returns false if no such pair is currently available - check GetLastPairStatusJson() for
	// why (left-only / right-only / skew too large / not yet paired).
	bool SaveStereoDetection();
	int GetPairCount() const;
	bool RemovePair(int index);
	void ClearPairs();

	// runs cv::stereoCalibrate + cv::stereoRectify over every saved pair; throws if fewer than 8
	// pairs have been saved (stereo extrinsics have more DOF than a single-eye calibration, so
	// the same "too few views is actively misleading" argument from CameraCalibrator applies
	// harder here - see STEREO_IMPLEMENTATION_PLAN.md ss10.2).
	StereoCalibrationResult RunCalibration();
	StereoCalibrationResult GetCalibrationResult() const;

private:
	void Process(std::vector<SourceResult> results) override;
	bool DetectCheckerboard(const cv::Mat& gray, std::vector<cv::Point2f>& corners, std::vector<cv::Point3f>& objectPoints);

	std::shared_ptr<Logger> m_Logger;
	StereoCalibrationBoardConfig m_BoardConfig;
	int64_t m_MaxSkewUs;

	std::string m_LeftSourceId, m_RightSourceId;

	// constructed once SetStereoRoles() supplies real source ids (StereoPairer.h - shared with
	// StereoDepthNode, which duplicated this exact pairing logic before ROADMAP.md Phase 3d).
	std::optional<StereoPairer> m_Pairer;

	mutable std::mutex m_DetectionMutex;
	bool m_LastPairFound = false;
	int64_t m_LastSkewUs = -1;
	bool m_LastFoundLeft = false, m_LastFoundRight = false;
	std::vector<cv::Point2f> m_LastLeftCorners, m_LastRightCorners;
	std::vector<cv::Point3f> m_LastObjectPoints;
	cv::Size m_LastFrameSize;

	std::vector<std::vector<cv::Point3f>> m_ObjPoints;
	std::vector<std::vector<cv::Point2f>> m_LeftImgPoints, m_RightImgPoints;
	cv::Size m_FrameSize;

	std::optional<CameraCalibrationResult> m_PriorLeft, m_PriorRight;
	std::optional<StereoCalibrationResult> m_LastResult;
};
