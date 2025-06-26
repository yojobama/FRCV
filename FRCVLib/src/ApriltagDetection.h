#pragma once
#include <apriltag/apriltag.h>
#include <apriltag/apriltag_pose.h>
#include <string>	
#include <sstream>

class ApriltagDetection
{
public:
	ApriltagDetection(apriltag_detection_t detection, apriltag_pose_t pose);
	~ApriltagDetection() = default;
	std::string toString();
private:
	apriltag_family_t* family;
	apriltag_detection_t* detection;
	apriltag_pose_t pose;
};

