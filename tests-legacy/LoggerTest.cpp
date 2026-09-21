#include "LoggerTest.h"
#include <cassert>

LoggerTest::LoggerTest() : UnitTestBase() {}
LoggerTest::~LoggerTest() {}

bool LoggerTest::innerTest() {
    Logger logger;
    logger.enterLog("test log");
    auto logs = logger.GetAllLogs();
    assert(!logs.empty());
    assert(logs[0].GetMessage() == "test log");
    return true;
}