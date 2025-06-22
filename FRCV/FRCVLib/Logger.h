#pragma once

#include <string>
#include <vector>
#include <bits/stdc++.h>
#include <iostream>
#include <mutex>

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
	void enterLog(Log* log);
	std::vector<Log*> *GetAllLogs();
	std::vector<Log*> GetCertainLogs(LogLevel logLevel);
	void clearAllLogs();
private:
	std::mutex lock;
	std::vector<Log*> logs;
};

