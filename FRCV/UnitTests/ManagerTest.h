#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/Manager.h"

class ManagerTest : public UnitTestBase {
public:
    ManagerTest();
    ~ManagerTest();
private:
    bool innerTest() override;
};