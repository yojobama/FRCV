#pragma once
#include "UnitTestBase.h"
#include "../FRCVLib/Manager.h"
#include <chrono>

class ManagerPerformanceTest : public UnitTestBase 
{
public:
    ManagerPerformanceTest();
    ~ManagerPerformanceTest();
private:
    bool innerTest() override;
    
    // Performance test helper methods
    bool testSinkCreationPerformance(Manager& manager);
    bool testSourceCreationPerformance(Manager& manager);
    bool testBindingPerformance(Manager& manager);
    bool testStatusQueryPerformance(Manager& manager);
    bool testSystemMonitoringPerformance(Manager& manager);
    bool testConcurrentOperations(Manager& manager);
    
    // Utility methods
    template<typename Func>
    double measureExecutionTime(Func func, int iterations = 1);
    
    void logPerformanceResult(const std::string& testName, double timeMs, int operations);
};