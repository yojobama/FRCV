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
	Logger(std::string filePath);
	~Logger();
	void enterLog(std::string message);
	void enterLog(LogLevel logLevel, std::string message);
	void enterLog(Log* log);
	void clearAllLogs();
	void flushLogs();
private:
	std::string filePath;
	std::mutex lock;
	std::vector<Log*> logs;
};

