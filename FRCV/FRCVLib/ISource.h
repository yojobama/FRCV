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
	ISource(FramePool* framePool, Logger* logger);
	virtual ~ISource();
	virtual std::shared_ptr<Frame> getLatestFrame();
	void changeThreadStatus(bool threadWantedAlive);
protected:
	virtual void captureFrame() = 0;
	FramePool* framePool;
	Logger* logger;
	FrameSpec frameSpec;
	std::queue<std::shared_ptr<Frame>> frames;
	bool doNotLoadThread = false;
private:
	static void* sourceThreadStart(void *pReference);
	void sourceThreadProc();
	std::mutex lock;
	pthread_t thread;
	bool shouldTerminate;
};

