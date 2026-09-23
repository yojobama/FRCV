#include <catch2/catch_test_macros.hpp>
#include "CpuApriltagBackend.h"
#include "VkApriltagBackend.h"
#include <opencv2/opencv.hpp>
#include <cmath>
#include <map>

// The acceptance gate ROADMAP.md Phase D flagged as missing entirely: CpuApriltagBackend and
// VkApriltagBackend must agree on the same input, not just each pass their own separate checks.
// A faster detector that disagrees with the CPU reference is useless - this is that check.
//
// Uses the real, already-hardware-validated reference image from the vkapriltag submodule itself
// (third_party/vkapriltag/apriltags_vulkan/grayimage.pgm, 1280x800, tag36h11) - the same file
// vkapriltag's own tools/validate_against_libapriltag tool was verified against on the real
// Orange Pi Mali G610 (0.58px corner RMS, matching tag IDs - see
// docs/history/IMPLEMENTATION_PLAN.md's phase 5 notes). Not a synthetic render: a real
// photographed tag is what that validation actually needs, and this file is already checked into
// git (part of the submodule), so no camera is needed to run this test.
//
// [hitl]-labelled and self-skips if no usable Vulkan compute device is found (every machine
// except the bench Orange Pi, or a dev box with a real GPU) - VkApriltagBackend's constructor
// throws in that case (see its own header comment), which is exactly the "skip, don't fail"
// signal every other hardware-dependent test in this suite already treats the same way.
TEST_CASE("CpuApriltagBackend and VkApriltagBackend agree on the same real image", "[hitl][vulkan][apriltag]") {
	const std::string pgmPath = std::string(LUMEN_VKAPRILTAG_SAMPLE_DIR) + "/grayimage.pgm";
	cv::Mat gray = cv::imread(pgmPath, cv::IMREAD_GRAYSCALE);
	REQUIRE_FALSE(gray.empty());

	// quad_decimate=2.0 on the CPU side to match VkApriltagBackend's own architecturally-fixed
	// 2x decimation (see VkApriltagBackend.cpp's own comment) - the same "put both backends on
	// equal footing" reasoning validate_against_libapriltag.cpp's own reference detector uses
	// (it explicitly sets quad_decimate=2.0 rather than leaving the CPU detector at full
	// resolution). Decimation trades detection range for speed, not final tag-id/pose accuracy
	// on tags actually found (apriltag's own binary payload decode always runs at full
	// resolution regardless), so this doesn't advantage either side.
	CpuApriltagBackend cpuBackend(/*nthreads*/1, /*quadDecimate*/2.0f);

	std::unique_ptr<VkApriltagBackend> vkBackend;
	try {
		vkBackend = std::make_unique<VkApriltagBackend>(gray.cols, gray.rows);
	} catch (const std::exception&) {
		SKIP("no usable Vulkan compute device on this machine - VkApriltagBackend falls back to "
			"CPU at the ApriltagDetector layer, but this test needs it to actually run standalone");
	}

	zarray_t* cpuDetections = cpuBackend.Detect(gray);
	zarray_t* vkDetections = vkBackend->Detect(gray);

	auto extractById = [](zarray_t* detections) {
		std::map<int, apriltag_detection_t> byId;
		for (int i = 0; i < zarray_size(detections); i++) {
			apriltag_detection_t* det;
			zarray_get(detections, i, &det);
			byId[det->id] = *det;
		}
		return byId;
	};

	std::map<int, apriltag_detection_t> cpuById = extractById(cpuDetections);
	std::map<int, apriltag_detection_t> vkById = extractById(vkDetections);

	REQUIRE_FALSE(cpuById.empty()); // the reference image is known to contain a real tag
	REQUIRE(cpuById.size() == vkById.size());

	double sumSquaredCornerError = 0.0;
	int comparedCorners = 0;
	for (const auto& [id, cpuDet] : cpuById) {
		INFO("checking tag id " << id);
		REQUIRE(vkById.count(id) == 1); // same tag IDs decoded by both backends
		const apriltag_detection_t& vkDet = vkById.at(id);
		for (int corner = 0; corner < 4; corner++) {
			double dx = cpuDet.p[corner][0] - vkDet.p[corner][0];
			double dy = cpuDet.p[corner][1] - vkDet.p[corner][1];
			sumSquaredCornerError += dx * dx + dy * dy;
			comparedCorners++;
		}
	}

	cpuBackend.ReleaseResult(cpuDetections);
	vkBackend->ReleaseResult(vkDetections);

	REQUIRE(comparedCorners > 0);
	double cornerRmsPx = std::sqrt(sumSquaredCornerError / comparedCorners);
	// the real, already-hardware-verified reference agreement was 0.58px on this exact image -
	// a generous margin above that (not a razor-thin exact match) so ordinary floating-point/
	// driver-version drift doesn't make this test flaky, while still catching a genuinely wrong
	// (not just slightly-differently-rounded) detection.
	REQUIRE(cornerRmsPx < 2.0);
}
