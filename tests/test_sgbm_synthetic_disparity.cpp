#include <catch2/catch_test_macros.hpp>
#include "SgbmStereoBackend.h"
#include <opencv2/opencv.hpp>
#include <algorithm>

// Golden-corpus regression coverage for SgbmStereoBackend (ROADMAP.md Phase D) - same synthetic-
// disparity technique already proven in test_codec_stereo_rkmpp_hitl.cpp (random-noise texture,
// cropped/shifted for a known horizontal disparity), pointed at cv::StereoSGBM instead. Pure
// OpenCV, no hardware/feature flag involved - runs everywhere, no [hitl] label needed.
TEST_CASE("SgbmStereoBackend recovers a known synthetic disparity", "[stereo][sgbm]") {
	const int W = 640, H = 384;
	const int SHIFT = 32; // known synthetic horizontal disparity, in pixels

	// Blocky texture (coarse random noise upscaled with nearest-neighbour), not per-pixel iid
	// noise - confirmed the hard way that pure per-pixel cv::randu() is a genuine worst case for
	// cv::StereoSGBM specifically (unlike the simple SAD-based motion search
	// CodecStereoBackend's tests use elsewhere): SGBM's whole strength is smoothness
	// regularization along multiple aggregation paths, which has nothing real to lock onto
	// against pixel-independent noise and produced a wildly spread, badly biased disparity map
	// (median ~25 against a true shift of 32, with valid disparities spanning 0..59). Coarse
	// blocks give correspondence enough real texture while still respecting local smoothness.
	const int texelSize = 8; // each coarse cell becomes an 8x8 block of identical pixels
	cv::Mat coarse((H + texelSize - 1) / texelSize, (W + SHIFT + texelSize - 1) / texelSize, CV_8UC1);
	cv::randu(coarse, 0, 255);
	cv::Mat base;
	cv::resize(coarse, base, cv::Size(W + SHIFT, H), 0, 0, cv::INTER_NEAREST);
	cv::Mat left = base(cv::Rect(SHIFT, 0, W, H)).clone();
	cv::Mat right = base(cv::Rect(0, 0, W, H)).clone();

	// Tightly bracket the known shift, matching how StereoDepthNode actually derives these from
	// a real min/max depth range (a wide arbitrary search range like [0,64) made the semi-global
	// smoothness prior wander badly - see this file's own texture-generation comment above).
	SgbmStereoBackend backend(/*blockW*/16, /*blockH*/16, /*minDisparity*/16, /*numDisparities*/32);
	REQUIRE(backend.Name() == "sgbm");

	std::vector<float> disparity;
	int cols = 0, rows = 0;
	REQUIRE(backend.Compute(left, right, disparity, cols, rows));
	REQUIRE(!disparity.empty());

	std::vector<float> validDisparities;
	validDisparities.reserve(disparity.size());
	for (float d : disparity) if (d != STEREO_DISPARITY_INVALID) validDisparities.push_back(d);
	REQUIRE(validDisparities.size() > disparity.size() / 2); // most blocks should resolve on pure texture

	// Median, not mean - StereoSGBM's semi-global smoothness prior has genuine per-block
	// ambiguity on synthetic (not naturally-structured) texture even with a tightly bracketed
	// search range, so a handful of outlier blocks pull a plain mean further than the bulk of
	// the distribution actually sits (confirmed empirically: p25/p75 cluster tightly around the
	// true shift while min/max spread wider) - median is the more robust central-tendency check,
	// same reasoning DepthFusionNode already uses for its own block-region distance estimate.
	std::nth_element(validDisparities.begin(), validDisparities.begin() + validDisparities.size() / 2, validDisparities.end());
	float medianDisparity = validDisparities[validDisparities.size() / 2];
	REQUIRE(medianDisparity > SHIFT - 5);
	REQUIRE(medianDisparity < SHIFT + 5);
}
