#include <catch2/catch_test_macros.hpp>
#include "ApriltagDetector.h"
#include "CameraCalibrationResult.h"
#include <chrono>
#include <thread>

// ROADMAP.md Phase 7: driver mode should still stream video but skip actual detection/NT4
// publish work entirely. Constructed directly (matching RoiSource's own test pattern) rather
// than through Manager, since the point here is ApriltagDetector's own Process() branch, not
// Manager's dynamic_pointer_cast dispatch (SinkController's REST layer is the thing that
// exercises that part, not unit-testable without a running server).

namespace {
class SyntheticFrameSource : public ISource {
public:
	SyntheticFrameSource(std::shared_ptr<Logger> logger, std::string id)
		: ISource(logger, id)
	{
	}

protected:
	void CaptureFrame() override {
		cv::Mat frame(240, 320, CV_8UC3, cv::Scalar(60, 90, 120));
		SetLatestResult(SourceResult(std::nullopt, frame));
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
};
}

TEST_CASE("ApriltagDetector's driver mode skips detection but still republishes frames", "[driver_mode]") {
	auto logger = std::make_shared<Logger>("LumenCoreTests-driver-mode.log");
	auto upstream = std::make_shared<SyntheticFrameSource>(logger, "driver-mode-upstream");

	// default-constructed CameraCalibrationResult (fx=fy=0) is deliberately "no real
	// calibration" - ApriltagDetector's own m_HasCalibration guard already handles that
	// gracefully (skips pose estimation), and driver mode returns even before that check runs.
	ApriltagDetector detector(logger, "driver-mode-detector", CameraCalibrationResult(), 0.1651);

	REQUIRE(detector.BindSource(upstream));
	detector.SetDriverMode(true);
	REQUIRE(detector.GetDriverMode());

	upstream->Toggle(true);
	static_cast<ISink&>(detector).Toggle(true);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	upstream->Toggle(false);
	static_cast<ISink&>(detector).Toggle(false);

	SourceResult result = detector.GetLatestResult();
	REQUIRE(result.frame.has_value());
	REQUIRE_FALSE(result.frame->empty());
	REQUIRE(result.json.has_value());
	// {"tags": [...], "multiTag": ...} - ApriltagDetector's real published envelope (ROADMAP.md
	// Phase 7's multi-tag PnP), not a bare array - both empty/null while driver mode is on.
	REQUIRE(result.json->is_object());
	REQUIRE(result.json->contains("tags"));
	REQUIRE((*result.json)["tags"].is_array());
	REQUIRE((*result.json)["tags"].empty()); // no detections published while driver mode is on
	REQUIRE((*result.json)["multiTag"].is_null());
}
