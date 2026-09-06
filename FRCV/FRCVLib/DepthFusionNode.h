#pragma once
#include "ISource.h"
#include "ISink.h"
#include "StereoDepthNode.h"
#include <memory>

// ISource+ISink, maxSources=1 - fuses a detector's (ObjectDetectionSink or ApriltagDetector)
// bounding boxes with a StereoDepthNode's block-grid depth, so a downstream consumer gets range
// alongside bearing: "note at 2.4m, 15 degrees left" instead of just a pixel bbox. See
// STEREO_IMPLEMENTATION_PLAN.md ss10.4.
//
// Only maxSources=1 (the detector) goes through the normal ISink::BindSource path - this node
// runs whenever a fresh detection result arrives, fused against whatever depth is CURRENTLY
// available, rather than needing its own left/right pairing logic. The StereoDepthNode is a
// second, direct C++ reference (SetStereoDepthNode), not a bound ISource: DepthFusionNode pulls
// the full depth grid in-process via StereoDepthNode::GetLastDepthGrid(), which deliberately
// never crosses through SourceResult/JSON (see that method's own comment for why).
//
// The detector MUST be bound to the depth node's own rectified-left frame output
// (StereoFrameOutput::STEREO_FRAME_RECTIFIED_LEFT), not the raw camera - otherwise its bbox
// pixel coordinates don't index into the same grid this node reads. That binding is the
// caller's responsibility (bind the detector's source to the StereoDepthNode's id, since it's
// dual-role); this node has no way to verify it.
class DepthFusionNode : public ISink, public ISource
{
public:
	DepthFusionNode(std::shared_ptr<Logger> logger, std::string id);

	void SetStereoDepthNode(std::shared_ptr<StereoDepthNode> depthNode);

private:
	void Process(std::vector<SourceResult> results) override;

	std::shared_ptr<Logger> m_Logger;
	std::shared_ptr<StereoDepthNode> m_DepthNode;
};
