#pragma once
#include "FilterBase.h"

#include <apriltag/apriltag.h>
#include <apriltag/apriltag_pose.h>
#include <apriltag/tag36h11.h>

class PreProcessor;
class FramePool;

// TODO: add some way to modify some detector parameters, such as decimation and nthreads
class ApriltagFilter : public FilterBase
{
public:
	ApriltagFilter(std::shared_ptr<Logger> logger, std::shared_ptr<FramePool> framePool, std::shared_ptr<PreProcessor> preProcessor);
	~ApriltagFilter() override;
	FilterAnalysis Process() override;
	std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> CreateDrawCommands(std::shared_ptr<FilterAnalysis> analysis) override;
private:
	apriltag_detector_t* m_Detector;
	std::shared_ptr<PreProcessor> m_PreProcessor;
	std::shared_ptr<FramePool> m_FramePool;
};

