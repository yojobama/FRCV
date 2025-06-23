#pragma once

#include <apriltag/apriltag.h>
#include "ISink.h"
#include "ApriltagDetection.h"
#include <vector>
#include "Logger.h"
#include <string>


class CameraCalibrationResult;
// Forward declare Logger to avoid header conflicts

class ApriltagSink : public ISink
{
public:
	ApriltagSink(Logger* logger);
    ~ApriltagSink();
	void addCameraInfo(CameraCalibrationResult cameraInfo);
	std::string getResults();
	std::string getStatus();
private:
	apriltag_family_t* family;
	apriltag_detector_t* detector;
	apriltag_detection_info_t info;
    Logger* logger;
};
