#include "WebRTCEndpoint.h"

#include "FilterAnalysis.h"
#include "Frame.h"
#include "DrawCommands.h"
#include "FramePool.h"

WebRTCEndpoint::WebRTCEndpoint(std::shared_ptr<Logger> logger, std::shared_ptr<FramePool> framePool) : EndpointBase(logger)
{
	m_FramePool = framePool;
}

WebRTCEndpoint::~WebRTCEndpoint()
{
}

void WebRTCEndpoint::ProcessFrame(std::vector<FilterAnalysis> analysisVector)
{
	std::shared_ptr<Frame> frame = m_FramePool->GetFrame(FrameSpec());
	RenderFrameWithAnalysisVector(frame, analysisVector);
	// Here you would typically send the frame via WebRTC
	m_FramePool->ReturnFrame(frame);
}

void WebRTCEndpoint::InitializePeerConnection()
{
}

void WebRTCEndpoint::RenderFrameWithAnalysisVector(std::shared_ptr<Frame> frame, std::vector<FilterAnalysis> analysisVector)
{
	for (FilterAnalysis analysis : analysisVector)
	{
		for (std::shared_ptr<DrawCommands::DrawCommandBase> command : analysis.GetDrawCommands()) 
		{
			command->Execute(frame);
		}
	}
}
