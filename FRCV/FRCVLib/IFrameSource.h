#pragma once
#include <opencv2/opencv.hpp>
#include "Logger.h"
#include "Frame.h"
#include "FramePool.h"
#include "FrameSpec.h"

class IFrameSource
{
public:
	IFrameSource(FramePool* framePool, Logger* logger);
	~IFrameSource() = default;
	virtual Frame* getFrame() = 0;
protected:
	FramePool* framePool;
	Logger* logger;
	FrameSpec frameSpec;
	// TODO: add basic functions and members (like a logger and such)
};

