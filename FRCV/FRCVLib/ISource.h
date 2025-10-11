#pragma once
#include "Logger.h"
#include "FramePool.h"
#include "FrameSpec.h"
#include <mutex>
#include <pthread.h>
#include <queue>

class Frame;

class ISource
{
public:
	ISource(FramePool* p_FramePool, Logger* p_Logger);
	virtual ~ISource();
	virtual std::shared_ptr<Frame> GetLatestFrame();
	virtual std::shared_ptr<Frame> GetLatestFrame(bool forceNewFrame);
	void ChangeThreadStatus(bool threadWantedAlive);
	uint64_t GetCurrentFrameCount();
	bool GetActivationStatus();
protected:
	uint64_t m_FrameCount = 0;
	virtual void CaptureFrame() = 0;
	FramePool* m_FramePool;
	Logger* m_Logger;
	FrameSpec m_FrameSpec;
	std::queue<std::shared_ptr<Frame>> m_Frames;
	bool m_DoNotLoadThread = false;
private:
	static void* SourceThreadStart(void* p_Reference);
	void SourceThreadProc();
	std::mutex m_Lock;
	pthread_t m_Thread;
	bool m_ShouldTerminate;
	bool m_Activated = false;
};

