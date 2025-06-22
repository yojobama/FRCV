#pragma once
#include "FramePool.h"
#include <opencv2/opencv.hpp>

class Frame;

class PreProcessor
{
public:
	PreProcessor(FramePool* framePool);
	~PreProcessor();
	Frame* transformFrame(Frame* src, FrameSpec spec);
private:
	FramePool* framePool;
};

