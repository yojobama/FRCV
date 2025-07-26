#include "Logger.h"
#include <memory.h>
#include <fstream>
#include <iostream>

// Constructor
Logger::Logger() {
    EnterLog("Logger constructed");
}

Logger::Logger(std::string filePath) {
    m_FilePath = filePath;
    EnterLog("Logger constructed with file path: " + filePath);
}

// Destructor
Logger::~Logger() {
    ClearAllLogs();
}

// Define the EnterLog method for a single string message
void Logger::EnterLog(std::string message) {
    std::lock_guard<std::recursive_mutex> guard(m_Lock);  // RAII lock
    m_Logs.push_back(new Log(LogLevel::Info, message));
    FlushLogs();
    std::cout << "[INFO]: " << message << std::endl; // Added console output for immediate feedback
}

// Define the EnterLog method for a log level and message
void Logger::EnterLog(LogLevel logLevel, std::string message) {
    std::lock_guard<std::recursive_mutex> guard(m_Lock);  // RAII lock
    m_Logs.push_back(new Log(logLevel, message));
    FlushLogs();
    std::cout << "[" << static_cast<int>(logLevel) << "]: " << message << std::endl; // Added console output for immediate feedback
}

// Define the EnterLog method for a Log object
void Logger::EnterLog(Log* p_Log) {
    if (!p_Log) return;
    std::lock_guard<std::recursive_mutex> guard(m_Lock);  // RAII lock
    m_Logs.push_back(new Log(p_Log->GetLogLevel(), p_Log->GetMessage()));
    if (m_Logs.size() > 100) {
        FlushLogs();
    }
}

// Define the method to clear all logs
void Logger::ClearAllLogs() {
    std::lock_guard<std::recursive_mutex> guard(m_Lock);  // RAII lock
    for (auto p_Log : m_Logs) {
        delete p_Log;
    }
    m_Logs.clear();
}

void Logger::FlushLogs()
{
    if (m_FilePath != "") {
        std::ofstream logFile(m_FilePath, std::ios::out | std::ios::app);

        if (logFile.is_open()) {
            std::lock_guard<std::recursive_mutex> guard(m_Lock);  // RAII lock
            while (!m_Logs.empty()) {
                logFile << "[" + m_Logs.back()->GetLogLevelString() + "]: " + m_Logs.back()->GetMessage() + "\n";
                m_Logs.pop_back();
            }
        }

        logFile.close();
    }
}


