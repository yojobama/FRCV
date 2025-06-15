#include "FramePoolTest.h"
#include <cassert>

FramePoolTest::FramePoolTest() : UnitTestBase() {
	framePool = new FramePool();
}

FramePoolTest::~FramePoolTest() {
	delete framePool;
}

bool FramePoolTest::innerTest() {
	assert(framePool != nullptr);
	// Add more tests as needed
	return true;
}