#pragma once
#include "Logger.h"
#include "FramePool.h"
#include "FrameSpec.h"
#include <mutex>
#include <pthread.h>

class Frame;

class ISource
{
public:
	ISource(FramePool* framePool, Logger* logger);
	virtual ~ISource();
	Frame* getFrame();
	void changeThreadStatus(bool threadWantedAlive);
protected:
	Frame* currentFrame;
	virtual void captureFrame() = 0;
	FramePool* framePool;
	Logger* logger;
	FrameSpec frameSpec;
private:
	static void* sourceThreadStart(void *pReference);
	void sourceThreadProc();
	std::mutex lock;
	pthread_t thread;
	bool shouldTerminate;
};

