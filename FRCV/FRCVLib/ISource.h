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
protected:
	virtual void captureFrame() = 0;
	FramePool* framePool;
	Logger* logger;
	FrameSpec frameSpec;
private:
	void threadStart(void *pReference);
	std::mutex lock;
	pthread_t thread;
	bool shouldTerminate = false;
	// TODO: add basic functions and members (like a logger and such)
};

