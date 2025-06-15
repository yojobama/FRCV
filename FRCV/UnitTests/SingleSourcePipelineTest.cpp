#include "SingleSourcePipelineTest.h"
#include <cassert>

SingleSourcePipelineTest::SingleSourcePipelineTest() : UnitTestBase() {}
SingleSourcePipelineTest::~SingleSourcePipelineTest() {}

bool SingleSourcePipelineTest::innerTest() {
    // Construction test
    assert(true);
    return true;
}