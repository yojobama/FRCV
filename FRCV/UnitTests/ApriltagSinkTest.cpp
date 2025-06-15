#include "ApriltagSinkTest.h"
#include <cassert>

ApriltagSinkTest::ApriltagSinkTest() : UnitTestBase() {}

ApriltagSinkTest::~ApriltagSinkTest() {}

bool ApriltagSinkTest::innerTest() {
    // Test constructor and destructor
    ApriltagSink* sink = new ApriltagSink(this->logger);
    assert(sink != nullptr);

    // Test getResults with a dummy frame
    Frame dummyFrame;
    dummyFrame.cols = 10;
    dummyFrame.rows = 10;
    unsigned char data[100] = {0};
    dummyFrame.data = data;

    // Should not crash, even if no tags are detected
    auto results = sink->getResults(&dummyFrame);
    assert(results.size() >= 0);

    delete sink;
    return true;
}