#include "ISource.h"

ISource::ISource(FramePool* framePool, Logger* logger)
	: frameSpec(0, 0, 0) // Initialize frameSpec with default values
{
	this->framePool = framePool;
	this->logger = logger;
}

// Provide a default implementation for the pure virtual function
Frame* ISource::getFrame()
{
	// Default implementation can return nullptr or log an error
	if (logger)
		logger->enterLog(LogLevel::ERROR, "getFrame() not implemented in derived class.");
	return nullptr;
}