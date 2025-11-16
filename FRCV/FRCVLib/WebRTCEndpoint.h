#pragma once
#include "EndpointBase.h"
#include <memory>
#include <rtc/rtc.hpp>

class Frame;

class WebRTCEndpoint : public EndpointBase
{
public:
	WebRTCEndpoint(std::shared_ptr<Logger> logger);
	~WebRTCEndpoint();

	void ProcessFrame(std::vector<FilterAnalysis> analysisVector) override;
private:
	void InitializePeerConnection();

	void RenderFrameWithAnalysis(std::shared_ptr<Frame> frame, std::shared_ptr<FilterAnalysis> analysis);

	std::shared_ptr<rtc::PeerConnection> m_PeerConnection;
};

