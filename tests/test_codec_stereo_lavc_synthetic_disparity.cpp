#include <catch2/catch_test_macros.hpp>
#include "CodecStereoBackend.h"
#include <opencv2/opencv.hpp>

// Golden-corpus coverage for codec-stereo's software (LAVC) motion-vector path - the RKMPP
// hardware path already has this exact test shape in test_codec_stereo_rkmpp_hitl.cpp; this is
// its non-hardware sibling. libavcodec's software motion estimation needs no RK3588 VPU, so this
// runs everywhere ffmpeg is available (already a WSL/CI dependency) - no [hitl] label needed.
TEST_CASE("codec-stereo's lavc_sw backend recovers a known synthetic disparity", "[stereo][lavc]") {
	const int W = 640, H = 384;
	const int SHIFT = 16; // known synthetic horizontal disparity, in pixels

	// random noise texture - motion estimation needs real texture to find correspondences; a
	// flat/solid image gives every block a meaningless zero-cost "match" everywhere.
	cv::Mat base(H, W + SHIFT, CV_8UC1);
	cv::randu(base, 0, 255);
	cv::Mat left = base(cv::Rect(SHIFT, 0, W, H)).clone();
	cv::Mat right = base(cv::Rect(0, 0, W, H)).clone();

	CodecStereoBackend::Config cfg;
	cfg.kind = STEREO_BACKEND_CODEC_LAVC;
	cfg.searchRangeX = 48;
	cfg.searchRangeY = 16;
	cfg.minDisparity = 0.0f;
	cfg.maxDy = 4;

	std::unique_ptr<CodecStereoBackend> backend;
	try {
		backend = std::make_unique<CodecStereoBackend>(cfg);
	} catch (const std::exception&) {
		SKIP("lavc_sw backend unavailable on this machine (codec-stereo not compiled with CS_ENABLE_LAVC)");
	}

	REQUIRE(backend->Name() == "lavc_sw");

	std::vector<float> disparity;
	int cols = 0, rows = 0;
	REQUIRE(backend->Compute(left, right, disparity, cols, rows));

	int validCount = 0;
	double sum = 0.0;
	for (float d : disparity) {
		if (d != STEREO_DISPARITY_INVALID) { validCount++; sum += d; }
	}
	REQUIRE(validCount > static_cast<int>(disparity.size()) / 2); // most blocks should resolve on pure texture
	double meanDisparity = sum / validCount;
	// within 2px of the known synthetic shift - motion search is block-quantized, not exact
	// (same tolerance as the RKMPP sibling test).
	REQUIRE(meanDisparity > SHIFT - 2);
	REQUIRE(meanDisparity < SHIFT + 2);
}
