#pragma once
#include <vector>
#include "Frame.h"
#include "FrameSpec.h"
#include "Logger.h"

class FramePool
{
public:
	FramePool(Logger* logger);
	~FramePool();
	int getCachedFrameCount();
	Frame* getFrame(FrameSpec frameSpec);
	void returnFrame(Frame* frame);
private:
	Frame* allocateFrame(FrameSpec frameSpec);
	std::vector<Frame*> frameVector;
	Logger* logger;
};

