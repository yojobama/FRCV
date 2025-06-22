#pragma once
#include <opencv2/opencv.hpp>
#include "Logger.h"
#include "Frame.h"
#include "FramePool.h"
#include "FrameSpec.h"

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
	// TODO: add basic functions and members (like a logger and such)
};

