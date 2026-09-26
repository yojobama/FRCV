#include "Logger.h"
#include <memory.h>
#include <fstream>
#include <iostream>
#include <cstdlib>

namespace {
	// Debug-level entries are dropped before anything else (no allocation, no mutex, no I/O) -
	// this is what makes ApriltagDetector::Process's own per-frame EnterLog(LogLevel::Debug, ...)
	// call free in normal operation (docs/PERFORMANCE_ANALYSIS.md's own §3 - the file-open/write/
	// close cycle below plus the process-wide mutex, paid on every detected frame, was a real
	// jitter source). Set LUMEN_LOG_DEBUG=1 to get them back for on-device troubleshooting -
	// matches this codebase's existing convention for opt-in verbose/expensive behaviour
	// (APRILTAG_VK_ALLOW_CPU, APRILTAG_CPU_THREADS). Checked once, not per call.
	bool DebugLoggingEnabled()
	{
		static const bool enabled = [] {
			const char* value = std::getenv("LUMEN_LOG_DEBUG");
			return value != nullptr && value[0] != '\0' && value[0] != '0';
		}();
		return enabled;
	}
}

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
    std::lock_guard<std::recursive_mutex> guard(m_ResultLock);  // RAII lock
    m_Logs.push_back(new Log(LogLevel::Info, message));
    FlushLogs();
    std::cout << "[INFO]: " << message << "\n";
}

// Define the EnterLog method for a log level and message
void Logger::EnterLog(LogLevel logLevel, std::string message) {
    // dropped before the lock/allocation/flush below - see DebugLoggingEnabled's own comment.
    if (logLevel == LogLevel::Debug && !DebugLoggingEnabled()) return;

    std::lock_guard<std::recursive_mutex> guard(m_ResultLock);  // RAII lock
    m_Logs.push_back(new Log(logLevel, message));
    FlushLogs();
    std::cout << "[" << static_cast<int>(logLevel) << "]: " << message << "\n";
}

// Define the EnterLog method for a Log object
void Logger::EnterLog(Log* p_Log) {
    if (!p_Log) return;
    std::lock_guard<std::recursive_mutex> guard(m_ResultLock);  // RAII lock
    m_Logs.push_back(new Log(p_Log->GetLogLevel(), p_Log->GetMessage()));
    if (m_Logs.size() > 100) {
        FlushLogs();
    }
}

// Define the method to clear all logs
void Logger::ClearAllLogs() {
    std::lock_guard<std::recursive_mutex> guard(m_ResultLock);  // RAII lock
    for (auto p_Log : m_Logs) {
        delete p_Log;
    }
    m_Logs.clear();
}

void Logger::FlushLogs()
{
    if (m_FilePath == "") return;

    std::lock_guard<std::recursive_mutex> guard(m_ResultLock);  // RAII lock

    // opened once and kept open for the Logger's lifetime, not reopened (fresh fopen/fwrite/
    // fclose) on every single call - that used to be a real per-frame disk-I/O cost on the
    // detector hot path (docs/PERFORMANCE_ANALYSIS.md's own §3).
    if (!m_LogFile.is_open()) {
        m_LogFile.open(m_FilePath, std::ios::out | std::ios::app);
        if (!m_LogFile.is_open()) return;
    }

    // front-to-back, not m_Logs.back()+pop - the previous version wrote newest-first, so the log
    // file read backwards from actual chronological order.
    for (Log* p_Log : m_Logs) {
        m_LogFile << "[" + p_Log->GetLogLevelString() + "]: " + p_Log->GetMessage() + "\n";
        delete p_Log;
    }
    m_Logs.clear();
}
