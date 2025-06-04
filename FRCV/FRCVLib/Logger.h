#pragma once

#include <string>

// TODO: add some stuff to make the logger functional
enum LogLevel
{
	INFO,
	WARNING,
	ERROR,
	DEBUG,
	WTF
};

class Logger
{
public:
	Logger();
	~Logger();
	void Log(std::string message);
	void Log(LogLevel logLevel, std::string message);
private:
};

