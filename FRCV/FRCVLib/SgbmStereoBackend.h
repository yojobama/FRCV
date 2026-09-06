#pragma once
#include "IStereoDepthBackend.h"

// cv::StereoSGBM, block-averaged onto the same cols x rows grid a codec-stereo backend would
// produce, so the two are directly comparable (STEREO_IMPLEMENTATION_PLAN.md ss0/ss10.6 item 4)
// and trivially swappable behind IStereoDepthBackend. Always compiled in (unlike
// CodecStereoBackend) - it has no dependency beyond OpenCV, which every FRCV configuration
// already links, and it is the accuracy reference / fallback if codec-stereo's numbers turn out
// not to be good enough on real FRC scenes.
class SgbmStereoBackend : public IStereoDepthBackend {
public:
	// numDisparities must be a positive multiple of 16 (cv::StereoSGBM's own requirement).
	SgbmStereoBackend(int blockW, int blockH, int minDisparity, int numDisparities);

	bool Compute(const cv::Mat& rectLeft, const cv::Mat& rectRight,
		std::vector<float>& disparityOut, int& cols, int& rows) override;

	std::string Name() const override { return "sgbm"; }
	int BlockW() const override { return m_BlockW; }
	int BlockH() const override { return m_BlockH; }

private:
	int m_BlockW, m_BlockH;
	int m_MinDisparity, m_NumDisparities;
	cv::Ptr<cv::StereoSGBM> m_Sgbm;
};
