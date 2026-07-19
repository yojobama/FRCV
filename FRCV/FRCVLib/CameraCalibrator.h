#pragma once
#include "ISource.h"
#include "ISink.h"
#include "CameraCalibrationResult.h"

#include <opencv2/calib3d.hpp>

const int CHECKERBOARD_WIDTH[2] = {6, 9};
const cv::Size patternSize(CHECKERBOARD_WIDTH[0], CHECKERBOARD_WIDTH[1]);

class CameraCalibrator : public ISource, public ISink
{
public:
	CameraCalibrator(std::shared_ptr<Logger> logger, std::string id);
	CameraCalibrationResult GetCalibrationResult();
private:
	void Process(std::vector<SourceResult> results) override;

	std::shared_ptr<Logger> m_Logger;

	// variables to store calibration data
	std::vector<std::vector<cv::Point3f>> m_ObjPoints;
	std::vector<std::vector<cv::Point2f>> m_ImgPoints;
	cv::Size frameSize;
};

