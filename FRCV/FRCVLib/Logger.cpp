#include "Logger.h"
#include <memory.h>
#include <fstream>
#include <iostream>

// Constructor
Logger::Logger() {
}

Logger::Logger(std::string filePath) {
    this->filePath = filePath;
}

// Destructor
Logger::~Logger() {
    clearAllLogs();
}

// Define the enterLog method for a single string message
void Logger::enterLog(std::string message) {
    std::lock_guard<std::mutex> guard(lock);  // RAII lock
    logs.push_back(new Log(INFO, message));
    if (logs.size() > 100) {
        flushLogs();
    }
}

// Define the enterLog method for a log level and message
void Logger::enterLog(LogLevel logLevel, std::string message) {
    std::lock_guard<std::mutex> guard(lock);  // RAII lock
    logs.push_back(new Log(logLevel, message));
    if (logs.size() > 100) {
        flushLogs();
    }
}

// Define the enterLog method for a Log object
void Logger::enterLog(Log *log) {
    if (!log) return;
    std::lock_guard<std::mutex> guard(lock);  // RAII lock
    logs.push_back(new Log(log->GetLogLevel(), log->GetMessage()));
    if (logs.size() > 100) {
        flushLogs();
    }
}

// Define the method to clear all logs
void Logger::clearAllLogs() {
    std::lock_guard<std::mutex> guard(lock);  // RAII lock
    for (auto log : logs) {
        delete log;
    }
    logs.clear();
}

void Logger::flushLogs()
{
    if (filePath != "") {
        std::ofstream logFile(filePath, std::ios::out | std::ios::app | std::ios::binary);

        if (logFile.is_open()) {
            std::lock_guard<std::mutex> guard(lock);  // RAII lock
            while (!logs.empty()) {
                logFile << "[" + logs.back()->GetLogLevelString() + "]: " + logs.back()->GetMessage() + "\n";
                logs.pop_back();
            }
        }

        logFile.close();
    }
}


