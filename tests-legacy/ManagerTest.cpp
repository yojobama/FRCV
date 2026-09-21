#include "ManagerTest.h"
#include <cassert>
#include <iostream>

ManagerTest::ManagerTest() = default;
ManagerTest::~ManagerTest() = default;

bool ManagerTest::innerTest() {
    // Test Manager construction and destruction
    Manager manager;
    logger->enterLog("ManagerTest: Manager constructed");

    // Test getAllSinks (should be empty initially)
    auto sinks = manager.getAllSinks();
    if (!sinks.empty()) {
        std::cerr << "ManagerTest failed: getAllSinks not empty on new manager." << std::endl;
        return false;
    }

    // Test getAllLogs (should be accessible)
    auto logs = manager.getAllLogs();
    logger->enterLog("ManagerTest: getAllLogs called");

    // Test clearAllLogs (should not throw)
    manager.clearAllLogs();
    logger->enterLog("ManagerTest: clearAllLogs called");

    // Test enumerateAvailableCameras (should not throw)
    auto cameras = manager.enumerateAvailableCameras();
    logger->enterLog("ManagerTest: enumerateAvailableCameras called");

    // Test createApriltagSink (should return an int)
    int sinkId = manager.createApriltagSink();
    logger->enterLog("ManagerTest: createApriltagSink called, id=" + std::to_string(sinkId));

    // Test getSinkStatusById (should not throw)
    std::string status = manager.getSinkStatusById(sinkId);
    logger->enterLog("ManagerTest: getSinkStatusById called");

    // Test getSinkResult (should not throw)
    std::string result = manager.getSinkResult(sinkId);
    logger->enterLog("ManagerTest: getSinkResult called");

    // If reached here, all tests passed
    return true;
}