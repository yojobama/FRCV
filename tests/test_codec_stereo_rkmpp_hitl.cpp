#include <catch2/catch_test_macros.hpp>
#include "CodecStereoBackend.h"
#include <opencv2/opencv.hpp>

// Hardware-in-the-loop coverage for ROADMAP.md Phase 6's other VPU win: codec-stereo's
// rkmpp_hwenc backend reads the RK3588 hardware H.264 encoder's own motion-vector search as a
// fast disparity proxy (see StereoDepthBackendKind.h's own comment on why rkmpp_hwenc, not the
// plain CS_ENABLE_RKMPP backend, is what this project selects). Self-skips anywhere
// CS_ENABLE_RKMPP_HWENC wasn't compiled in (every machine except the bench Orange Pi once
// install-deps.sh --with-mpp has run) - CodecStereoBackend's constructor throws if cs_init()
// can't get the requested backend, which is exactly the failure this test treats as "skip", not
// "fail": an unavailable optional hardware backend is not a defect on a machine that never had
// the hardware to begin with.
TEST_CASE("codec-stereo's rkmpp_hwenc backend recovers a known synthetic disparity from real VPU hardware", "[hitl][rkmpp]") {
	const int W = 640, H = 384; // multiple of 32x16 - rkmpp_hwenc forces that block size regardless of Config
	const int SHIFT = 16;       // known synthetic horizontal disparity, in pixels

	// random noise texture - motion estimation needs real texture to find correspondences; a
	// flat/solid image gives every block a meaningless zero-cost "match" everywhere.
	cv::Mat base(H, W + SHIFT, CV_8UC1);
	cv::randu(base, 0, 255);
	cv::Mat left = base(cv::Rect(SHIFT, 0, W, H)).clone();
	cv::Mat right = base(cv::Rect(0, 0, W, H)).clone();

	CodecStereoBackend::Config cfg;
	cfg.kind = STEREO_BACKEND_CODEC_RKMPP_HWENC;
	cfg.searchRangeX = 48;
	cfg.searchRangeY = 16;
	cfg.minDisparity = 0.0f;
	cfg.maxDy = 4;

	std::unique_ptr<CodecStereoBackend> backend;
	try {
		backend = std::make_unique<CodecStereoBackend>(cfg);
	} catch (const std::exception&) {
		SKIP("rkmpp_hwenc backend unavailable on this machine (no RK3588 VPU, or rockchip_mpp not installed - see install-deps.sh --with-mpp)");
	}

	REQUIRE(backend->Name() == "rkmpp_hwenc");

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
	REQUIRE(meanDisparity > SHIFT - 2);
	REQUIRE(meanDisparity < SHIFT + 2);
}
