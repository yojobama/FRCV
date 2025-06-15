#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/CameraFrameSource.h"

class CameraFrameSourceTest : public UnitTestBase {
public:
    CameraFrameSourceTest();
    ~CameraFrameSourceTest();
private:
    bool innerTest() override;
};