#pragma once
#include "ISink.h"
#include <opencv2/calib3d.hpp>
#include <apriltag_pose.h>


class CameraCalibrationSink : ISink<CameraCalibrationResult>
{
public:
	CameraCalibrationSink(Logger* logger) : ISink<CameraCalibrationResult>(logger) {}
	~CameraCalibrationSink();
	std::vector<CameraCalibrationResult> getResults(Frame* frame);
};

