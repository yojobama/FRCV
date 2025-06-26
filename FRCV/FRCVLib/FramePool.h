#pragma once
#include <vector>
#include "FrameSpec.h"
#include "Logger.h"
#include <mutex>
#include <memory>

class Frame;

class FramePool
{
public:
	FramePool(Logger* logger);
	~FramePool();
	int getCachedFrameCount();
	std::shared_ptr<Frame> getFrame(FrameSpec frameSpec);
	void returnFrame(std::shared_ptr<Frame> frame);
private:
	std::shared_ptr<Frame> allocateFrame(FrameSpec frameSpec);
	std::vector<std::shared_ptr<Frame>> frameVector;
	Logger* logger;
	std::mutex lock;
};

