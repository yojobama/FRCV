#pragma once
#include "ISource.h"
#include "ISink.h"
#include "CameraCalibrationResult.h"
#include "CalibrationBoardType.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/objdetect/charuco_detector.hpp>
#include <mutex>
#include <optional>

struct CalibrationBoardConfig {
	CalibrationBoardType type = BOARD_CHECKERBOARD;
	// interior corner count for a checkerboard, square count (rows x cols) for a ChArUco board
	int rows = 6;
	int cols = 9;
	float squareSizeMeters = 0.025f;
	// ChArUco only, ignored for a plain checkerboard
	float markerSizeMeters = 0.018f;
	// cv::aruco::PredefinedDictionaryType numeric value (e.g. DICT_6X6_250 == 10); left as a
	// plain int rather than that enum so this struct doesn't need aruco_dictionary.h's full
	// enum wrapped through SWIG for one field
	int arucoDictionaryId = 10;
};

class CameraCalibrator : public ISource, public ISink
{
public:
	CameraCalibrator(std::shared_ptr<Logger> logger, std::string id, CalibrationBoardConfig boardConfig = CalibrationBoardConfig());

	// runs cv::calibrateCamera over every snapshot saved so far and caches the result; throws
	// if fewer than 4 snapshots have been saved (cv::calibrateCamera's own practical minimum
	// for a stable solution, not enforced by OpenCV itself but by us, since a 1-3 view result
	// is numerically unstable enough to be actively misleading rather than just imprecise)
	CameraCalibrationResult RunCalibration();
	// returns the last result RunCalibration() produced, or an empty result if it hasn't run yet
	CameraCalibrationResult GetCalibrationResult() const;

	// saves the checkerboard/ChArUco corners detected in the most recently processed frame as
	// a calibration snapshot to be used by RunCalibration(). returns false if no board was
	// detected yet.
	bool SaveBoardDetection();
	int GetSnapshotCount() const;
	// removes one saved snapshot by index (0-based, in the order SaveBoardDetection() was
	// called); returns false if index is out of range
	bool RemoveSnapshot(int index);
	void ClearSnapshots();

private:
	void Process(std::vector<SourceResult> results) override;
	void ProcessCheckerboard(const cv::Mat& gray, cv::Mat& displayFrame);
	void ProcessCharuco(const cv::Mat& gray, cv::Mat& displayFrame);

	std::shared_ptr<Logger> m_Logger;
	CalibrationBoardConfig m_BoardConfig;

	// ChArUco-only: constructed once from m_BoardConfig, since CharucoBoard/CharucoDetector are
	// stateless with respect to individual frames
	std::optional<cv::aruco::CharucoBoard> m_CharucoBoard;
	std::optional<cv::aruco::CharucoDetector> m_CharucoDetector;

	// variables to store calibration data
	std::vector<std::vector<cv::Point3f>> m_ObjPoints;
	std::vector<std::vector<cv::Point2f>> m_ImgPoints;
	cv::Size frameSize;

	// state of the most recent board detection, populated by Process() and consumed by SaveBoardDetection()
	mutable std::mutex m_DetectionMutex;
	bool m_LastPatternFound = false;
	std::vector<cv::Point2f> m_LastCorners;
	std::vector<cv::Point3f> m_LastObjectPoints; // ChArUco: matched per-detection, not a fixed grid
	cv::Size m_LastFrameSize;

	std::optional<CameraCalibrationResult> m_LastResult;
};
