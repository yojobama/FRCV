#pragma once
#include "ISink.h"
#include "ISource.h"
#include <apriltag/apriltag.h>

class FramePool;
class Logger;
class PreProcessor;

class ApriltagDetector : ISink, ISource
{
public:
	ApriltagDetector(FramePool* framePool, Logger* logger, PreProcessor* preProcessor);
	~ApriltagDetector();



private:
	apriltag_detector_t* detector;
	apriltag_family_t* family;

	void CaptureFrame();
};

