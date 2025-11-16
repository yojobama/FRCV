#pragma once
#include "EndpointBase.h"
#include <memory>
#include <rtc/rtc.hpp>
#include <vector>

class FramePool;
class Frame;

class WebRTCEndpoint : public EndpointBase
{
public:
	WebRTCEndpoint(std::shared_ptr<Logger> logger, std::shared_ptr<FramePool> framePool);
	~WebRTCEndpoint();

	void ProcessFrame(std::vector<FilterAnalysis> analysisVector) override;
private:
	void InitializePeerConnection();

	void RenderFrameWithAnalysisVector(std::shared_ptr<Frame> frame, std::vector<FilterAnalysis> analysisVector);

	std::shared_ptr<rtc::PeerConnection> m_PeerConnection;
	std::shared_ptr<FramePool> m_FramePool;
};

