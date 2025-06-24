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
	Frame* getLatestFrame();
	void changeThreadStatus(bool threadWantedAlive);
	void returnFrameToPool(Frame* frame);
protected:
	virtual void captureFrame() = 0;
	FramePool* framePool;
	Logger* logger;
	FrameSpec frameSpec;
	std::queue<Frame*> frames;
private:
	static void* sourceThreadStart(void *pReference);
	void sourceThreadProc();
	std::mutex lock;
	pthread_t thread;
	bool shouldTerminate;
};

