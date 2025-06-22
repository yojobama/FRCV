#pragma once
#include <vector>
#include "FrameSpec.h"
#include "Logger.h"

class Frame;

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

