#pragma once
#ifdef LUMEN_WITH_CODEC_STEREO
#include "IStereoDepthBackend.h"
#include "StereoDepthBackendKind.h"
#include <cstdint>

struct cs_context; // codec_stereo/cs.h - kept out of this header, matching how VkApriltagBackend.h
                    // and OnnxDetectionBackend.h keep their own third-party types out of theirs

// Wraps codec-stereo's cs_extract() - see STEREO_IMPLEMENTATION_PLAN.md ss10.3. Owns one
// cs_context for its whole lifetime (one persistent encoder/decoder context per node, per
// codec-stereo's own design - creating one per frame is what its "context churn" finding in
// Sec. 9a of its design doc measured as a ~24ms/call cost).
class CodecStereoBackend : public IStereoDepthBackend {
public:
	struct Config {
		StereoDepthBackendKind kind = STEREO_BACKEND_CODEC_AUTO;
		int blockW = 16, blockH = 16;               // rkmpp_hwenc forces 32x16 regardless - see cs.h's
		int searchRangeX = 48, searchRangeY = 16;    // caps.mv_min_x/max_x and STEREO_IMPLEMENTATION_PLAN's worked example
		int32_t disparityOffset = 0;                 // pre-shift so the search window covers min..max depth - see StereoDepthNode
		bool invertDisparitySign = false;             // resolved once by StereoDepthNode's startup self-check

		// gating passed straight through to cs_mv_field_to_disparity - derived by StereoDepthNode
		// from minDepthMeters/maxDepthMeters (min_disparity) and left as quality knobs (max_dy,
		// max_cost); a min_disparity <= 0 (e.g. an R->L cross-check pass, not used by this node
		// today) must come with the near-zero gate effectively disabled - see cs_mv_field's own
		// "min_disparity's near-zero gate" caveat and STEREO_IMPLEMENTATION_PLAN.md ss10.3.
		float minDisparity = 0.0f;
		int maxDy = 4;
		uint16_t maxCost = 0; // 0 = no cost gate
	};

	explicit CodecStereoBackend(const Config& cfg);
	~CodecStereoBackend() override;

	bool Compute(const cv::Mat& rectLeft, const cv::Mat& rectRight,
		std::vector<float>& disparityOut, int& cols, int& rows) override;

	std::string Name() const override;
	int BlockW() const override { return m_Cfg.blockW; }
	int BlockH() const override { return m_Cfg.blockH; }

private:
	Config m_Cfg;
	cs_context* m_Ctx = nullptr;
};
#endif // LUMEN_WITH_CODEC_STEREO
