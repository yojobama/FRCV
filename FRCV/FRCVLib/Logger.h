#pragma once

#include <string>
#include <vector>
#include <bits/stdc++.h>
#include <iostream>
#include <mutex>
#include <memory>

// TODO: add some stuff to make the logger functional
enum LogLevel
{
	INFO,
	WARNING,
	DEBUG,
	ERROR,
	WTF
};

class Log {
public:
	Log(LogLevel logLevel, std::string message)
		: logLevel(logLevel), message(message) {}
	LogLevel GetLogLevel() { return logLevel; }
	std::string GetMessage() { return message; }
	std::string GetLogLevelString() {
		switch (logLevel) {
			case INFO: return "INFO";
			case WARNING: return "WARNING";
			case DEBUG: return "DEBUG";
			case ERROR: return "ERROR";
			case WTF: return "WTF";
			default: return "UNKNOWN";
		}
	}
private:
	LogLevel logLevel;
	std::string message;
};

class Logger
{
public:
	Logger();
	~Logger();
	void enterLog(std::string message);
	void enterLog(LogLevel logLevel, std::string message);
	void enterLog(std::unique_ptr<Log> log);
	std::vector<std::unique_ptr<Log>> GetAllLogs();
	std::vector<std::unique_ptr<Log>> GetCertainLogs(LogLevel logLevel);
	void clearAllLogs();
private:
	std::mutex lock;
	std::vector<std::unique_ptr<Log>> logs;
};

