#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/ApriltagDetection.h"

class ApriltagDetectionTest : public UnitTestBase {
public:
    ApriltagDetectionTest();
    ~ApriltagDetectionTest();
private:
    bool innerTest() override;
};