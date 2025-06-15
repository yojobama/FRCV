#include "IFrameSourceTest.h"
#include <cassert>

IFrameSourceTest::IFrameSourceTest() : UnitTestBase() {}
IFrameSourceTest::~IFrameSourceTest() {}

bool IFrameSourceTest::innerTest() {
    // Construction test
    assert(true);
    return true;
}