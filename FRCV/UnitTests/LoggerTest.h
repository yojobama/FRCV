#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/Logger.h"

class LoggerTest : public UnitTestBase {
public:
    LoggerTest();
    ~LoggerTest();
private:
    bool innerTest() override;
};