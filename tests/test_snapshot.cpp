#include <catch2/catch_test_macros.hpp>
#include "Manager.h"
#include <filesystem>
#include <opencv2/imgcodecs.hpp>

// ROADMAP.md Phase 7: Manager::SaveSnapshot writes a source's latest frame to disk. bus.jpg
// (already a repo test fixture - see test_rknn_detection_hitl.cpp) publishes synchronously in
// ImageFileFrameSource's own constructor (no capture thread, no wait needed - see its own
// comment), which is what makes this a same-call round trip rather than needing a toggle+sleep.
TEST_CASE("Manager::SaveSnapshot writes a real, readable image file", "[snapshot]") {
	Manager manager;
	std::string busJpgPath = std::string(LUMEN_TEST_DATA_DIR) + "/bus.jpg";
	int sourceId = manager.CreateImageFileSource(busJpgPath);

	std::filesystem::path outPath = std::filesystem::temp_directory_path() / "lumencore-test-snapshot.png";
	std::filesystem::remove(outPath); // in case a previous run left it behind

	REQUIRE(manager.SaveSnapshot(sourceId, outPath.string()));
	REQUIRE(std::filesystem::exists(outPath));

	cv::Mat written = cv::imread(outPath.string());
	REQUIRE_FALSE(written.empty());
	// bus.jpg is a known 640x640 fixture (see test_rknn_detection_hitl.cpp's own comment)
	REQUIRE(written.cols == 640);
	REQUIRE(written.rows == 640);

	std::filesystem::remove(outPath);
}

TEST_CASE("Manager::SaveSnapshot returns false for a source that hasn't published a frame", "[snapshot]") {
	Manager manager;
	// no calibration attached, no camera bound - GetAllSinks/etc aside, an ApriltagDetector
	// bound to nothing never calls SetLatestResult, so its GetLatestResult().frame stays unset.
	int sinkId = manager.CreateApriltagDetector();

	std::filesystem::path outPath = std::filesystem::temp_directory_path() / "lumencore-test-snapshot-empty.png";
	REQUIRE_FALSE(manager.SaveSnapshot(sinkId, outPath.string()));
	REQUIRE_FALSE(std::filesystem::exists(outPath));
}
