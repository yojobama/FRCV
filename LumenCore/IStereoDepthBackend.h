#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// Shared invalid-cell marker for every backend's disparity output, deliberately matching
// codec-stereo's own CS_DISPARITY_INVALID (cs.h) so StereoDepthNode's gating/rendering code is
// identical regardless of which backend produced a given cell - and so this header doesn't need
// to depend on cs.h (only CodecStereoBackend.cpp, built under LUMEN_WITH_CODEC_STEREO, does).
#define STEREO_DISPARITY_INVALID (-1.0f)

// Detection-only-style backend abstraction, same shape as IApriltagBackend/IDetectionBackend:
// every implementation hands back disparity on the same block grid so StereoDepthNode's
// rectification, disparity-window derivation, JSON emission and frame annotation stay identical
// regardless of which backend produced the numbers. This is also what makes CodecStereoBackend
// and SgbmStereoBackend directly comparable (STEREO_IMPLEMENTATION_PLAN.md ss0/ss10.6 item 4) -
// the whole point of shipping SGBM alongside codec-stereo rather than after it.
class IStereoDepthBackend {
public:
	virtual ~IStereoDepthBackend() = default;

	// rectLeft/rectRight must already be rectified, single-channel (CV_8UC1), same size, and a
	// size the backend's native block size divides evenly - StereoDepthNode is responsible for
	// cropping to that before calling in (never padding - see the plan's note on why a synthetic
	// black border is actively harmful to a motion-vector based backend).
	//
	// On success, fills disparityOut with cols*rows floats (block-raster order, CS_DISPARITY_
	// INVALID-equivalent for invalid cells - see cs.h) and sets cols/rows, and returns true.
	virtual bool Compute(const cv::Mat& rectLeft, const cv::Mat& rectRight,
		std::vector<float>& disparityOut, int& cols, int& rows) = 0;

	virtual std::string Name() const = 0;
	virtual int BlockW() const = 0;
	virtual int BlockH() const = 0;
};
