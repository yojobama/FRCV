#include "Logger.h"
#include "Manager.h"

// Constructor
Logger::Logger() {
}

// Destructor
Logger::~Logger() {}

// Define the enterLog method for a single string message
void Logger::enterLog(std::string message) {
    lock.lock();
    logs.push_back(new Log(INFO, message)); // Add the log to the logs vector
    lock.unlock();
    std::cout << "[INFO]: " << message << std::endl; // Print the log to the console
}

// Define the enterLog method for a log level and message
void Logger::enterLog(LogLevel logLevel, std::string message) {
    lock.lock();
	Log log(logLevel, message);
    logs.push_back(&log); // Add the log to the logs vector
    lock.unlock();
	log.GetLogLevelString(); // Print the log to the console
}

// Define the enterLog method for a Log object
void Logger::enterLog(Log *log) {
    lock.lock();
    logs.push_back(log); // Add the log to the logs vector
    lock.unlock();
    std::cout << "[" << log->GetLogLevel() << "]: " << log->GetMessage() << std::endl; // Print the log to the console
}

// Define the method to get all logs
std::vector<Log*> *Logger::GetAllLogs() {
    return &logs;
}

// Define the method to get logs of a specific log level
std::vector<Log*> Logger::GetCertainLogs(LogLevel logLevel) {
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
    logs.clear();
}


