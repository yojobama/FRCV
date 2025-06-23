#include "ISource.h"
#include "Frame.h"

ISource::ISource(FramePool* framePool, Logger* logger)
	: frameSpec(0, 0, 0) // Initialize frameSpec with default values
{
	this->framePool = framePool;
	this->logger = logger;
}

ISource::~ISource()
{
}

// Provide a default implementation for the pure virtual function
Frame* ISource::getFrame()
{
	return currentFrame;
}

void ISource::changeThreadStatus(bool threadWantedAlive)
{
	if (threadWantedAlive) {
		shouldTerminate = false;
		pthread_create(&thread, NULL, sourceThreadStart, this);
	}
	else {
		if (thread) {
			shouldTerminate = true;
			while (shouldTerminate) sleep(1);
		}
	}
}

void* ISource::sourceThreadStart(void* pReference)
{
	((ISource*)pReference)->sourceThreadProc();
	return NULL;
}

void ISource::sourceThreadProc()
{
	while (!shouldTerminate) {
		captureFrame();
	}
	shouldTerminate = false;
}
