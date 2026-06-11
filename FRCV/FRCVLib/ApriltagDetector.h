#pragma once
#include "ISink.h"
#include "ISource.h"
#include <apriltag/apriltag.h>
#include <apriltag/tag36h11.h>

class FramePool;
class Logger;

class ApriltagDetector : public ISink, public ISource
{
public:
	ApriltagDetector(std::shared_ptr<Logger> logger, std::string id);
	~ApriltagDetector();
private:
	void Process(std::vector<SourceResult> results) override;

	apriltag_detector_t* m_Detector;
	apriltag_family_t* m_Family;

	std::shared_ptr<Logger> m_Logger;
};

