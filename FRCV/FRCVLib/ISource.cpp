#include "ISource.h"
#include "Frame.h"

ISource::ISource(FramePool* framePool, Logger* logger)
	: frameSpec(0, 0, 0), lock() // Initialize frameSpec with default values
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
	if (!frames.empty()) {
		frames.back()->reference();
		bool hasReferences = frames.front()->getReferences() == 0;
		while (!hasReferences) {
			framePool->returnFrame(frames.front());
			if (!frames.empty())
			{
				frames.pop();
				if (frames.front() != nullptr) {
					hasReferences = frames.front()->getReferences() == 0;
				}
				hasReferences = true;
			}
			else {
				hasReferences = true;
			}
		}
		Frame* frame = frames.back();
		lock.unlock();
		return frame;
	} else {
		logger->enterLog(ERROR, "frame queue is empty");
		lock.unlock();
		return nullptr;
	}
}

void ISource::changeThreadStatus(bool threadWantedAlive)
{
	if (threadWantedAlive && !doNotLoadThread) {
		shouldTerminate = false;
		pthread_create(&thread, NULL, sourceThreadStart, this);
	}
	else if (!doNotLoadThread) {
		if (thread) {
			shouldTerminate = true;
			while (shouldTerminate);
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
