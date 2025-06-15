#include "ISinkTest.h"
#include <cassert>

ISinkTest::ISinkTest() : UnitTestBase() {}
ISinkTest::~ISinkTest() {}

bool ISinkTest::innerTest() {
    // Construction test
    assert(true);
    return true;
}