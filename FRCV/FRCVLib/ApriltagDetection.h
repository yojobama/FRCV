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
	std::string ToString();

	const apriltag_family_t* m_pFamily;
	const apriltag_detection_t* m_pDetection;
	const apriltag_pose_t m_Pose;
};

