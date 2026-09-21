#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/PreProcessor.h"

class PreProcessorTest : public UnitTestBase {
public:
    PreProcessorTest();
    ~PreProcessorTest();
private:
    bool innerTest() override;
};