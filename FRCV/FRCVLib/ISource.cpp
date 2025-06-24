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

Frame* ISource::getLatestFrame()
{
	lock.lock();
	frames.back()->reference();
	bool hasReferences = frames.front()->getReferences() == 0;
	while (!hasReferences) {
		framePool->returnFrame(frames.front());
		if (!frames.empty())
		{
			frames.pop();
			hasReferences = frames.front()->getReferences() == 0;
		}
		else {
			hasReferences = true;
		}
	}
	Frame* frame = frames.back();
	lock.unlock();
	return frame;
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
