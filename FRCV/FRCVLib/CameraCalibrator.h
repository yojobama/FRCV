#pragma once
#include "ISource.h"
#include "ISink.h"
#include "CameraCalibrationResult.h"

#include <opencv2/calib3d.hpp>
#include <mutex>

const int CHECKERBOARD_WIDTH[2] = {6, 9};
const cv::Size patternSize(CHECKERBOARD_WIDTH[0], CHECKERBOARD_WIDTH[1]);
const float CHECKERBOARD_SQUARE_SIZE_METERS = 0.025f; // physical size of a single checkerboard square

class CameraCalibrator : public ISource, public ISink
{
public:
	CameraCalibrator(std::shared_ptr<Logger> logger, std::string id);
	CameraCalibrationResult GetCalibrationResult();

	// saves the checkerboard corners detected in the most recently processed frame as a calibration
	// snapshot to be used later by GetCalibrationResult(). returns false if no board was detected yet.
	bool SaveBoardDetection();
private:
	void Process(std::vector<SourceResult> results) override;

	std::shared_ptr<Logger> m_Logger;

	// variables to store calibration data
	std::vector<std::vector<cv::Point3f>> m_ObjPoints;
	std::vector<std::vector<cv::Point2f>> m_ImgPoints;
	cv::Size frameSize;

	// state of the most recent board detection, populated by Process() and consumed by SaveBoardDetection()
	std::mutex m_DetectionMutex;
	bool m_LastPatternFound = false;
	std::vector<cv::Point2f> m_LastCorners;
	cv::Size m_LastFrameSize;
};

