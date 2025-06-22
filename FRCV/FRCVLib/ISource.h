#pragma once
#include "Logger.h"
#include "FramePool.h"
#include "FrameSpec.h"
#include <mutex>

class Frame;

class ISource
{
public:
	ISource(FramePool* framePool, Logger* logger);
	virtual ~ISource() = default;
	virtual Frame* getFrame();
protected:
	FramePool* framePool;
	Logger* logger;
	FrameSpec frameSpec;
	std::mutex lock;
	// TODO: add basic functions and members (like a logger and such)
};

