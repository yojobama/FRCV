#pragma once
#include "ISource.h"
#include "ISink.h"

#include <opencv2/calib3d.hpp>

using namespace cv;

const int CHECKERBOARD_WIDTH[2] = {6, 9};

class CameraCalibrator : public ISource, public ISink
{
public:
	CameraCalibrator(Logger* logger);
private:
	// variables to store calibration data
	std::vector<std::vector<cv::Point3f>> m_ObjPoints;
	std::vector<std::vector<cv::Point2f>> m_ImgPoints;

	// functions for calibration
	bool findChessboardCorners(InputArray image, Size patternSize, OutputArray corners, int flags = CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE);
	void cornerSubPix(InputArray image, InputOutputArray corners, Size winSize, Size zeroZone, TermCriteria criteria);
	double calibrateCamera(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints, Size imageSize, InputOutputArray cameraMatrix, InputOutputArray distCoeffs, OutputArrayOfArrays rvecs, OutputArrayOfArrays tvecs);
};

