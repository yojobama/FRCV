#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/IFrameSource.h"

class IFrameSourceTest : public UnitTestBase {
public:
    IFrameSourceTest();
    ~IFrameSourceTest();
private:
    bool innerTest() override;
};