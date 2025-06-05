#pragma once

#include <apriltag/apriltag.h>
#include "ISink.h"
#include "ApriltagDetection.h"
#include <vector>
#include "Logger.h"

// Forward declare Logger to avoid header conflicts

class ApriltagSink : public ISink<ApriltagDetection>
{
public:
	ApriltagSink(Logger* logger);
    ~ApriltagSink();
	std::vector<ApriltagDetection> getDetections();
private:
	apriltag_family_t* family;
	apriltag_detector_t* detector;
	apriltag_detection_info_t info;
    Logger logger;
};
