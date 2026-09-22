#include "DepthFusionNode.h"
#include <algorithm>
#include <cmath>

DepthFusionNode::DepthFusionNode(std::shared_ptr<Logger> logger, std::string id)
	: ISink(logger, 1, true, false, id), ISource(logger, id), m_Logger(logger)
{
	m_DoNotLoadCaptureThread = true;
}

void DepthFusionNode::SetStereoDepthNode(std::shared_ptr<StereoDepthNode> depthNode)
{
	m_DepthNode = depthNode;
}

void DepthFusionNode::Process(const std::vector<SourceResult>& results)
{
	if (results.empty() || !results[0].json.has_value()) return;
	if (!m_DepthNode) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "DepthFusionNode: no StereoDepthNode attached - call SetStereoDepthNode first.");
		return;
	}

	std::vector<float> depth;
	int cols, rows, blockW, blockH;
	if (!m_DepthNode->GetLastDepthGrid(depth, cols, rows, blockW, blockH)) return;

	StereoCalibrationResult calibration = m_DepthNode->GetCalibration();
	double fx = calibration.rectifiedFx, cx = calibration.rectifiedCx, cy = calibration.rectifiedCy;

	// annotate-on-demand - see ApriltagDetector::Process's identical comment. Skips the clone
	// AND every distance-label cv::putText call below when nothing bound to this sink actually
	// wants the frame.
	bool wantsFrame = HasActiveFrameConsumer();

	cv::Mat annotatedFrame;
	if (wantsFrame && results[0].frame.has_value() && !results[0].frame->empty()) annotatedFrame = results[0].frame->AsBgr().clone();

	std::vector<nlohmann::json> fused;

	for (const auto& detection : results[0].json.value()) {
		if (!detection.contains("box") || detection["box"].size() != 4) continue;
		double bx = detection["box"][0], by = detection["box"][1];
		double bw = detection["box"][2], bh = detection["box"][3];

		int blockX0 = std::max(0, (int)(bx / blockW));
		int blockY0 = std::max(0, (int)(by / blockH));
		int blockX1 = std::min(cols, (int)std::ceil((bx + bw) / blockW));
		int blockY1 = std::min(rows, (int)std::ceil((by + bh) / blockH));

		std::vector<float> validDepths;
		for (int by2 = blockY0; by2 < blockY1; by2++) {
			for (int bx2 = blockX0; bx2 < blockX1; bx2++) {
				float d = depth[(size_t)by2 * cols + bx2];
				if (d > 0.0f) validDepths.push_back(d);
			}
		}

		// median, not mean - a single background block bleeding into the box wrecks a mean far
		// more than it wrecks a median. See STEREO_IMPLEMENTATION_PLAN.md ss10.4.
		double distanceMeters = 0.0;
		double validFraction = validDepths.empty() ? 0.0 : (double)validDepths.size() / ((blockX1 - blockX0) * (blockY1 - blockY0));
		if (!validDepths.empty()) {
			std::nth_element(validDepths.begin(), validDepths.begin() + validDepths.size() / 2, validDepths.end());
			distanceMeters = validDepths[validDepths.size() / 2];
		}

		// pixel-center pinhole projection, approximating fy=fx (true after rectification for
		// OpenCV's stereoRectify output, which shares one focal length across both P1 columns
		// bar rounding) - not the full Q-matrix reprojection, but equivalent to it for a point
		// already known to lie on the rectified left image plane at this pixel.
		double u = bx + bw / 2.0, v = by + bh / 2.0;
		double xMeters = (fx > 0 && distanceMeters > 0) ? (u - cx) * distanceMeters / fx : 0.0;
		double yMeters = (fx > 0 && distanceMeters > 0) ? (v - cy) * distanceMeters / fx : 0.0;

		nlohmann::json out = detection;
		out["distanceMeters"] = distanceMeters;
		out["xMeters"] = xMeters;
		out["yMeters"] = yMeters;
		out["depthValidFraction"] = validFraction;
		fused.push_back(out);

		if (!annotatedFrame.empty() && distanceMeters > 0.0) {
			std::string label = std::to_string(distanceMeters).substr(0, 4) + "m";
			cv::putText(annotatedFrame, label, cv::Point((int)bx, (int)(by + bh) + 15),
				cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0xff, 0xff), 1);
		}
	}

	SetLatestResult(SourceResult(nlohmann::json(fused), annotatedFrame));
}
