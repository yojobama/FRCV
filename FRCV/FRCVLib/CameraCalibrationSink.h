#pragma once
#include "ISink.h"

class CameraCalibrationResult {

};

class CameraCalibrationSink : ISink<CameraCalibrationResult>
{
public:
	CameraCalibrationSink();
	~CameraCalibrationSink();
	std::vector<CameraCalibrationResult> getResults(Frame* frame);
};

