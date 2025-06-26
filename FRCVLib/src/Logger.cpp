#include "Logger.h"

// Constructor
Logger::Logger() {
}

// Destructor
Logger::~Logger() {
    clearAllLogs();
}

// Define the enterLog method for a single string message
void Logger::enterLog(std::string message) {
    std::lock_guard<std::mutex> guard(lock);  // RAII lock
    logs.push_back(new Log(INFO, message));
}

// Define the enterLog method for a log level and message
void Logger::enterLog(LogLevel logLevel, std::string message) {
    std::lock_guard<std::mutex> guard(lock);  // RAII lock
    logs.push_back(new Log(logLevel, message));
}

// Define the enterLog method for a Log object
void Logger::enterLog(Log *log) {
    if (!log) return;
    std::lock_guard<std::mutex> guard(lock);  // RAII lock
    logs.push_back(new Log(log->GetLogLevel(), log->GetMessage()));
}

// Define the method to get all logs
std::vector<Log*> *Logger::GetAllLogs() {
    return &logs;
}

// Define the method to get logs of a specific log level
std::vector<Log*> Logger::GetCertainLogs(LogLevel logLevel) {
    std::lock_guard<std::mutex> guard(lock);  // RAII lock
    std::vector<Log*> filteredLogs;
    for (const auto& log : logs) {
        if (log->GetLogLevel() == logLevel) {
            filteredLogs.push_back(log);
        }
    }
    return filteredLogs;
}

// Define the method to clear all logs
void Logger::clearAllLogs() {
    std::lock_guard<std::mutex> guard(lock);  // RAII lock
    for (auto log : logs) {
        delete log;
    }
    logs.clear();
}


