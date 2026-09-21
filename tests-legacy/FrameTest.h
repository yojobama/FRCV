#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/Frame.h"

class FrameTest : public UnitTestBase {
public:
    FrameTest();
    ~FrameTest();
private:
    bool innerTest() override;
};