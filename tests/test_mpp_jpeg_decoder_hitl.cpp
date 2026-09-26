#include <catch2/catch_test_macros.hpp>
#include "MppJpegDecoder.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>

// Hardware-in-the-loop coverage for the RK3588 JPEG-decode VPU path (MppJpegDecoder), isolated
// from the rest of the pipeline - no camera, no Server, no systemd service at risk. Runs as part
// of the normal ctest suite (including on CI's own arm64 runner, which has rockchip_mpp built but
// no real dma-heap/JPEG-VPU hardware - EnsureInitialized()/SetupBufferGroup() are expected to
// fail cleanly there, not crash; the real board is what actually exercises a working decode).
// LUMEN_TEST_DATA_DIR/bus.jpg is decoded once via the existing software cv::imdecode path first,
// purely to learn its real width/height (Decode() needs the caller to already know these, exactly
// like V4l2CameraBackend::Grab() already does from the negotiated V4L2 mode) - not to compare
// pixels against, since a JPEG-VPU decode and libjpeg-turbo's own YCbCr->RGB math are never
// bit-identical (different rounding), only visually equivalent.

TEST_CASE("MppJpegDecoder decodes a real JPEG on real RK3588 JPEG-VPU hardware", "[hitl][mpp]") {
	std::ifstream file(std::string(LUMEN_TEST_DATA_DIR) + "/bus.jpg", std::ios::binary);
	REQUIRE(file.is_open());
	std::vector<uint8_t> jpegBytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE_FALSE(jpegBytes.empty());

	cv::Mat reference = cv::imdecode(jpegBytes, cv::IMREAD_COLOR);
	REQUIRE_FALSE(reference.empty());
	int width = reference.cols, height = reference.rows;

	MppJpegDecoder decoder;

	SECTION("colour (BGR) output") {
		cv::Mat dst(height, width, CV_8UC3);
		bool ok = decoder.Decode(jpegBytes.data(), jpegBytes.size(), width, height, /*asGray=*/false, dst);
		if (!ok) {
			SKIP("MppJpegDecoder::Decode returned false - no working JPEG VPU on this machine (expected on CI/non-RK3588); this test only confirms real behaviour on actual hardware.");
		}
		REQUIRE(dst.rows == height);
		REQUIRE(dst.cols == width);
		REQUIRE(dst.type() == CV_8UC3);
		// visual sanity, not bit-exactness (see this file's own comment) - the average brightness
		// of a real photo decoded two different ways should be close; a wildly different mean
		// (e.g. near-zero or near-255, or channels swapped into a very different mean) would
		// indicate a genuinely wrong decode (bad stride, wrong plane order), not just rounding.
		cv::Scalar meanRef = cv::mean(reference);
		cv::Scalar meanDst = cv::mean(dst);
		for (int c = 0; c < 3; c++) {
			INFO("channel " << c << ": reference mean=" << meanRef[c] << " decoded mean=" << meanDst[c]);
			REQUIRE(std::abs(meanRef[c] - meanDst[c]) < 25.0);
		}
	}

	SECTION("grayscale (Y-plane) output") {
		cv::Mat referenceGray;
		cv::cvtColor(reference, referenceGray, cv::COLOR_BGR2GRAY);

		cv::Mat dst(height, width, CV_8UC1);
		bool ok = decoder.Decode(jpegBytes.data(), jpegBytes.size(), width, height, /*asGray=*/true, dst);
		if (!ok) {
			SKIP("MppJpegDecoder::Decode returned false - no working JPEG VPU on this machine (expected on CI/non-RK3588); this test only confirms real behaviour on actual hardware.");
		}
		REQUIRE(dst.rows == height);
		REQUIRE(dst.cols == width);
		REQUIRE(dst.type() == CV_8UC1);
		cv::Scalar meanRef = cv::mean(referenceGray);
		cv::Scalar meanDst = cv::mean(dst);
		INFO("reference gray mean=" << meanRef[0] << " decoded gray mean=" << meanDst[0]);
		REQUIRE(std::abs(meanRef[0] - meanDst[0]) < 25.0);
	}

	SECTION("repeated decode on the same instance does not crash or leak state") {
		// the real-world call pattern: one long-lived MppJpegDecoder, many frames - exercises the
		// buffer-group's regrow-if-needed path staying a no-op once already sized, not just a
		// single info-change round trip.
		cv::Mat dst(height, width, CV_8UC3);
		bool ok = true;
		for (int i = 0; i < 10 && ok; i++) {
			ok = decoder.Decode(jpegBytes.data(), jpegBytes.size(), width, height, false, dst);
		}
		if (!ok) {
			SKIP("MppJpegDecoder::Decode returned false - no working JPEG VPU on this machine (expected on CI/non-RK3588).");
		}
		REQUIRE(dst.rows == height);
		REQUIRE(dst.cols == width);
	}
}
