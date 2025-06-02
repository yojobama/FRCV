#pragma once
#include <vector>
#include "Frame.h"
#include "FrameSpec.h"

class FramePool
{
public:
	FramePool(std::vector<FrameSpec> initialSpecs);
	~FramePool();
	Frame* getFrame(FrameSpec frameSpec);
private:
	void allocateFrame(FrameSpec frameSpec);
	std::vector<Frame*> frameVector;
};

