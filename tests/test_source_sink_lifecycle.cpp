#include <catch2/catch_test_macros.hpp>
#include "ISource.h"
#include "ISink.h"
#include <atomic>
#include <chrono>
#include <thread>

// Regression coverage for this project's std::jthread migration (raw pthread_t before it): the
// two bugs that migration fixed for free - ISource's destructor never joining its capture
// thread, and Toggle(false) needing to return promptly rather than hang - had NO test coverage
// before, so a future refactor could silently reintroduce either with nothing to catch it.

namespace {

class TestSource : public ISource {
public:
    TestSource(std::shared_ptr<Logger> logger, std::string id) : ISource(logger, id) {}
    std::atomic<int> captureCount{0};

protected:
    void CaptureFrame() override {
        captureCount++;
        SetLatestResult(SourceResult(std::nullopt, cv::Mat(1, 1, CV_8UC1), SourceResult::NowUs()));
        // avoid pegging a CPU core spinning as fast as possible - a real capture backend blocks
        // on I/O between frames, this just needs to yield regularly instead
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
};

class TestSink : public ISink {
public:
    TestSink(std::shared_ptr<Logger> logger, std::string id)
        : ISink(logger, /*maxSources*/ 1, /*requireJson*/ false, /*requireFrame*/ true, id) {}
    std::atomic<int> processedResultCount{0};

protected:
    void Process(std::vector<SourceResult> results) override {
        processedResultCount += static_cast<int>(results.size());
    }
};

} // namespace

TEST_CASE("a bound, toggled-on source/sink pair actually moves frames end to end", "[ISource][ISink]") {
    auto source = std::make_shared<TestSource>(nullptr, "test-source");
    auto sink = std::make_shared<TestSink>(nullptr, "test-sink");

    REQUIRE(sink->BindSource(source));

    source->Toggle(true);
    sink->Toggle(true);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    source->Toggle(false);
    sink->Toggle(false);

    REQUIRE(source->captureCount.load() > 0);
    REQUIRE(sink->processedResultCount.load() > 0);
}

TEST_CASE("ISource::Toggle(false) returns promptly instead of hanging", "[ISource][regression]") {
    auto source = std::make_shared<TestSource>(nullptr, "test-source-stop");
    source->Toggle(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    auto start = std::chrono::steady_clock::now();
    source->Toggle(false);
    auto elapsed = std::chrono::steady_clock::now() - start;

    // generous bound (the capture loop itself only sleeps 1ms between frames) - this is a
    // "didn't hang forever" check, not a tight timing assertion
    REQUIRE(elapsed < std::chrono::seconds(2));
    REQUIRE_FALSE(source->GetToggleStatus());
}

TEST_CASE("ISink::Toggle(false) returns promptly instead of hanging", "[ISink][regression]") {
    auto sink = std::make_shared<TestSink>(nullptr, "test-sink-stop");
    sink->Toggle(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    auto start = std::chrono::steady_clock::now();
    sink->Toggle(false);
    auto elapsed = std::chrono::steady_clock::now() - start;

    REQUIRE(elapsed < std::chrono::seconds(2));
    REQUIRE_FALSE(sink->GetToggleStatus());
}

TEST_CASE("a source destroyed while its capture thread is still running does not hang or crash", "[ISource][regression]") {
    // Previously a no-op: ~ISource() never joined the capture thread at all, so this either
    // leaked a thread running against a half-destroyed object or (depending on timing) crashed
    // outright - confirmed as a real bug this project's own std::jthread migration fixed.
    auto start = std::chrono::steady_clock::now();
    {
        auto source = std::make_shared<TestSource>(nullptr, "test-source-destroy");
        source->Toggle(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        // source goes out of scope here while still toggled on
    }
    auto elapsed = std::chrono::steady_clock::now() - start;
    REQUIRE(elapsed < std::chrono::seconds(2));
}

TEST_CASE("calling Toggle(true) twice does not leak a second capture thread", "[ISource][regression]") {
    // ISource::Toggle's own comment documents this exact historical bug: calling Toggle(true)
    // while already running used to spawn a second capture thread without stopping the first,
    // silently overwriting the handle needed to ever join the original.
    auto source = std::make_shared<TestSource>(nullptr, "test-source-idempotent");
    source->Toggle(true);
    source->Toggle(true); // should be a no-op, not a second thread
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    auto start = std::chrono::steady_clock::now();
    source->Toggle(false);
    auto elapsed = std::chrono::steady_clock::now() - start;

    REQUIRE(elapsed < std::chrono::seconds(2));
}
