#include "FrameTest.h"
#include <cassert>

FrameTest::FrameTest() : UnitTestBase() {}
FrameTest::~FrameTest() {}

bool FrameTest::innerTest() {
    Frame f;
    f.cols = 5;
    f.rows = 5;
    unsigned char data[25] = {0};
    f.data = data;
    assert(f.cols == 5 && f.rows == 5);
    assert(f.data == data);
    return true;
}