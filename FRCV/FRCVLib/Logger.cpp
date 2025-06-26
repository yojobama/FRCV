#include "Logger.h"
#include <memory.h>

// Constructor
Logger::Logger() {}

// Destructor
Logger::~Logger() {}

// Define the enterLog method for a single string message
void Logger::enterLog(std::string message) {
    std::lock_guard<std::mutex> guard(lock);
    logs.emplace_back(std::make_unique<Log>(LogLevel::INFO, message));
}

// Define the enterLog method for a log level and message
void Logger::enterLog(LogLevel logLevel, std::string message) {
    std::lock_guard<std::mutex> guard(lock);
    logs.emplace_back(std::make_unique<Log>(logLevel, message));
}

// Define the enterLog method for a Log object
void Logger::enterLog(std::unique_ptr<Log> log) {
    std::lock_guard<std::mutex> guard(lock);
    logs.emplace_back(std::move(log)); // Use std::move to transfer ownership
}

// Define the method to get all logs
std::vector<std::unique_ptr<Log>> Logger::GetAllLogs() {
    std::lock_guard<std::mutex> guard(lock);
    std::vector<std::unique_ptr<Log>> allLogs;
    for (auto& log : logs) {
        allLogs.emplace_back(std::make_unique<Log>(log->GetLogLevel(), log->GetMessage()));
    }
    return allLogs;
}

// Define the method to get logs of a specific log level
std::vector<std::unique_ptr<Log>> Logger::GetCertainLogs(LogLevel logLevel) {
    std::lock_guard<std::mutex> guard(lock);
    std::vector<std::unique_ptr<Log>> filteredLogs;
    for (auto& log : logs) {
        if (log->GetLogLevel() == logLevel) {
            filteredLogs.emplace_back(std::make_unique<Log>(log->GetLogLevel(), log->GetMessage()));
        }
    }
    return filteredLogs;
}

// Define the method to clear all logs
void Logger::clearAllLogs() {
    std::lock_guard<std::mutex> guard(lock);
    logs.clear();
}


