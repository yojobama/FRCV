#pragma once
#include "FramePool.h"
#include <memory>

class Frame;
class Logger;

class PreProcessor
{
public:
	PreProcessor(FramePool* framePool);
	~PreProcessor();
	std::shared_ptr<Frame> transformFrame(std::shared_ptr<Frame> src, FrameSpec spec);
private:
	Logger* logger;
	FramePool* framePool;
};

