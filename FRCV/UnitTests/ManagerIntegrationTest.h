#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/Manager.h"

class ManagerIntegrationTest : public UnitTestBase 
{
public:
    ManagerIntegrationTest();
    ~ManagerIntegrationTest();
private:
    bool innerTest() override;
    
    // Helper methods for testing specific scenarios
    bool testCameraSourceWorkflow(Manager& manager);
    bool testVideoFileWorkflow(Manager& manager);
    bool testImageFileWorkflow(Manager& manager);
    bool testMultipleSinksWorkflow(Manager& manager);
    bool testSystemMonitoringWorkflow(Manager& manager);
    bool testPreviewWorkflow(Manager& manager);
    bool testCameraCalibrationWorkflow(Manager& manager);
    bool testErrorHandling(Manager& manager);
};