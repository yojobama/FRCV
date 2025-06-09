#pragma once
#include <vector>
#include "Frame.h"
#include "FrameSpec.h"
#include "Logger.h"

class FramePool
{
public:
	FramePool(std::vector<FrameSpec> initialSpecs, Logger* logger);
	~FramePool();
	Frame* getFrame(FrameSpec frameSpec);
	void returnFrame(Frame* frame);
private:
	Frame* allocateFrame(FrameSpec frameSpec);
	std::vector<Frame*> frameVector;
	Logger* logger;
};

