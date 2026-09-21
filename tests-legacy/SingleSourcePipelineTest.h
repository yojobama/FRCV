#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/SingleSourcePipeline.h"

class SingleSourcePipelineTest : public UnitTestBase {
public:
    SingleSourcePipelineTest();
    ~SingleSourcePipelineTest();
private:
    bool innerTest() override;
};