#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/ISink.h"

class ISinkTest : public UnitTestBase {
public:
    ISinkTest();
    ~ISinkTest();
private:
    bool innerTest() override;
};