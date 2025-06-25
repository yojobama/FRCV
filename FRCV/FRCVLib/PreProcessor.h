#pragma once
#include "FramePool.h"

class Frame;
class Logger;

class PreProcessor
{
public:
	PreProcessor(FramePool* framePool);
	~PreProcessor();
	Frame* transformFrame(Frame* src, FrameSpec spec);
private:
	Logger* logger;
	FramePool* framePool;
};

