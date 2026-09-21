#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/ApriltagSink.h"
#include "../FRCVLib/Frame.h"

class ApriltagSinkTest : public UnitTestBase
{
public:
    ApriltagSinkTest();
    ~ApriltagSinkTest();
private:
    bool innerTest() override;
};