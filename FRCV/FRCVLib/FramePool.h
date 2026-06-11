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
	FramePool(std::shared_ptr<Logger> p_Logger);
	~FramePool();
	int GetCachedFrameCount();
	std::shared_ptr<Frame> GetFrame(FrameSpec frameSpec);
	void ReturnFrame(std::shared_ptr<Frame> p_Frame);
private:
	std::shared_ptr<Frame> AllocateFrame(FrameSpec frameSpec);
	std::vector<std::shared_ptr<Frame>> m_FrameVector;
	std::shared_ptr<Logger> m_Logger;
	std::mutex m_ResultLock;
};

