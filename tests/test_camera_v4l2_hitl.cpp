#include <catch2/catch_test_macros.hpp>
#include "Manager.h"
#include "CameraSource.h"
#include <chrono>
#include <thread>

// Hardware-in-the-loop coverage for ROADMAP.md Phase 3b (explicit resolution control) - the only
// piece of this project's test suite that needs a real camera, so it self-skips everywhere one
// isn't attached (WSL, the Windows dev box today) rather than failing there. Verified for real
// against the bench Lenovo UVC camera on the Orange Pi (Manager::EnumerateAvailableCameras'
// V4L2_CAP_VIDEO_CAPTURE/STREAMING capability check correctly resolves it past the 4 video nodes
// the device exposes - see ROADMAP.md's note on dropping the even-device-number heuristic).
TEST_CASE("V4L2 camera backend enumerates real modes and honours an explicit SetMode request", "[hitl][camera]") {
    Manager manager;
    auto cameras = manager.EnumerateAvailableCameras();
    if (cameras.empty()) {
        SKIP("no camera hardware available on this machine");
    }

    int sourceId = manager.CreateCameraSource(cameras[0]);

    auto modes = manager.GetCameraModes(sourceId);
    REQUIRE_FALSE(modes.empty());

    REQUIRE(manager.SetCameraMode(sourceId, modes[0]));

    CameraMode current = manager.GetCameraCurrentMode(sourceId);
    REQUIRE(current.width == modes[0].width);
    REQUIRE(current.height == modes[0].height);
}

// Goes one step further than the SetMode test above: actually starts the capture thread and
// confirms real frames arrive through the full V4L2 mmap/poll/DQBUF path (not just that ioctls
// succeed) - constructed directly against CameraFrameSource rather than through Manager, since
// Manager has no public GetLatestResult passthrough and this test links LumenCore directly anyway.
TEST_CASE("V4L2 camera backend actually captures frames once started", "[hitl][camera]") {
    Manager manager;
    auto cameras = manager.EnumerateAvailableCameras();
    if (cameras.empty()) {
        SKIP("no camera hardware available on this machine");
    }

    auto logger = std::make_shared<Logger>("LumenCoreTests-hitl.log");
    CameraFrameSource source(cameras[0].path, cameras[0].name, logger, "hitl-capture-test");

    source.Toggle(true);
    // give the capture thread a few real frame intervals to actually produce something, even at
    // the slowest end of the modes this camera advertises (5fps, confirmed on the bench device).
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    source.Toggle(false);

    REQUIRE(source.GetCurrentFrameCount() > 0);

    SourceResult result = source.GetLatestResult();
    REQUIRE(result.frame.has_value());
    REQUIRE_FALSE(result.frame->empty());
}
