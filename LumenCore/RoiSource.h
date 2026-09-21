#pragma once
#include "ISink.h"
#include "ISource.h"

// Splits one upstream source's frames into a fixed rectangular crop, republishing each as its
// own independent frame - ROADMAP.md Phase 3d's generic building block for a side-by-side (or
// top-bottom) stereo camera: bind two RoiSources (one per eye's half of the frame) to one
// upstream CameraFrameSource, then bind each RoiSource into StereoCalibrator/StereoDepthNode
// exactly like two real, independent cameras. Deliberately does NOT change StereoCalibrator/
// StereoDepthNode's own two-source binding model at all - a RoiSource just makes one physical
// camera present as two ordinary sources to the rest of the pipeline.
//
// Uses SourceResult's three-argument constructor to propagate the upstream captureTimeUs
// unchanged (not the two-argument overload, which would substitute publish time instead) - a
// two-argument construction here would silently break the one guarantee stereo pairing depends
// on: two independently-bound RoiSources must report the SAME captureTimeUs for corresponding
// halves of what was, upstream, a single physical exposure.
class RoiSource : public ISink, public ISource
{
public:
	RoiSource(std::shared_ptr<Logger> logger, std::string id, cv::Rect roi);

private:
	void Process(std::vector<SourceResult> results) override;

	std::shared_ptr<Logger> m_Logger;
	cv::Rect m_Roi;
};
