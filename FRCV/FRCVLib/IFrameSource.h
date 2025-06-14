#pragma once
#include <opencv2/opencv.hpp>
#include "Logger.h"
#include "Frame.h"

class IFrameSource
{
public:
	IFrameSource() = default;
	~IFrameSource() = default;
	virtual Frame* getFrame();
private:
	Logger logger;
	// TODO: add basic functions and members (like a logger and such)
};

