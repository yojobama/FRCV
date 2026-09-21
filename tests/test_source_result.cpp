#include <catch2/catch_test_macros.hpp>
#include "SourceResult.h"

TEST_CASE("SourceResult::NowUs is a plausible, monotonically non-decreasing wall clock", "[SourceResult]") {
    uint64_t first = SourceResult::NowUs();
    uint64_t second = SourceResult::NowUs();
    // 1.7e15 us ~= 2023-11-14 - a loose sanity floor, not a real epoch check; catches "this
    // returned 0" or "this returned milliseconds/nanoseconds instead of microseconds" outright,
    // which is exactly the class of bug that would silently break the skew gate stereo pairing
    // depends on (StereoCalibrator/StereoDepthNode compare captureTimeUs values in microseconds).
    REQUIRE(first > 1'700'000'000'000'000ULL);
    REQUIRE(second >= first);
}

TEST_CASE("SourceResult's two-argument constructor leaves captureTimeUs at its honest-unknown default", "[SourceResult]") {
    // The "captureTimeUs==0 means unknown, fall back to producedTimeUs" substitution is
    // ISource::SetLatestResult's job (confirmed by reading it), not SourceResult's own
    // constructor - a producer that hasn't gone through SetLatestResult yet must still see 0
    // here, not some already-substituted value, or a future refactor could accidentally move
    // that substitution into the constructor and silently break producers that construct a
    // SourceResult without ever publishing it (there are none today, but this pins the contract).
    SourceResult result(std::nullopt, cv::Mat());
    REQUIRE(result.captureTimeUs == 0);
    REQUIRE(result.frameNumber == 0);
}

TEST_CASE("SourceResult's three-argument constructor records an explicit capture timestamp", "[SourceResult]") {
    SourceResult result(std::nullopt, cv::Mat(), 12345ULL);
    REQUIRE(result.captureTimeUs == 12345ULL);
}

TEST_CASE("SourceResult round-trips optional json and frame fields", "[SourceResult]") {
    // a bare cv::Mat implicitly becomes a BGR24-tagged Frame (see Frame's own constructor
    // comment), so a 3-channel mat here mirrors what every real producer actually passes -
    // pixel access goes through Frame::AsBgr(), not a member Frame no longer exposes.
    nlohmann::json j = {{"id", 7}};
    cv::Mat m(4, 4, CV_8UC3, cv::Scalar(42, 42, 42));
    SourceResult result(j, m);

    REQUIRE(result.json.has_value());
    REQUIRE((*result.json)["id"] == 7);
    REQUIRE(result.frame.has_value());
    REQUIRE(result.frame->AsBgr().at<cv::Vec3b>(0, 0) == cv::Vec3b(42, 42, 42));
}
