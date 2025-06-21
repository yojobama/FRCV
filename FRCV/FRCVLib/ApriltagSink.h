#pragma once

#include <apriltag/apriltag.h>
#include "ISink.h"
#include "ApriltagDetection.h"
#include <vector>
#include "Logger.h"
#include <string>

// Forward declare Logger to avoid header conflicts

class ApriltagSink : public ISink
{
public:
	ApriltagSink(Logger* logger);
    ~ApriltagSink();
	std::vector<std::string> getResults();
private:
	apriltag_family_t* family;
	apriltag_detector_t* detector;
	apriltag_detection_info_t info;
    Logger* logger;
};
