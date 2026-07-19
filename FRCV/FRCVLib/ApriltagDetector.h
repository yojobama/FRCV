#pragma once
#include "ISink.h"
#include "ISource.h"
#include <apriltag/apriltag.h>
#include <apriltag/apriltag_pose.h>
#include <apriltag/tag36h11.h>

class FramePool;
class Logger;
class CameraCalibrationResult;

class ApriltagDetector : public ISink, public ISource
{
public:
	ApriltagDetector(std::shared_ptr<Logger> logger, std::string id, CameraCalibrationResult calibrationResult, double tagSize /* in METERS you bloody Americans */);
	~ApriltagDetector();
private:
	void Process(std::vector<SourceResult> results) override;

	apriltag_detector_t* m_Detector;
	apriltag_family_t* m_Family;

	std::shared_ptr<Logger> m_Logger;
	apriltag_detection_info_t m_DetectionInfo;
};

