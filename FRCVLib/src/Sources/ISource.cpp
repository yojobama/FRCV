#include "ISource.h"
#include "../Frame.h"

ISource::ISource(FramePool* framePool, Logger* logger)
	: frameSpec(0, 0, 0), lock() // Initialize frameSpec with default values
{
	this->framePool = framePool;
	this->logger = logger;
}

ISource::~ISource()
{
}

std::shared_ptr<Frame> ISource::getLatestFrame()
{
	std::lock_guard<std::mutex> guard(lock); // Use RAII for mutex locking

	while (!frames.empty()) {
		std::shared_ptr<Frame> frontFrame = frames.front();
		if (frontFrame.use_count() > 1) {
			// Frame is still in use, return the latest frame
			return frames.back();
		} else {
			// Remove unused frame from the queue
			frames.pop();
		}
	}

	// If no valid frames are available, log an error and return nullptr
	logger->enterLog(ERROR, "Frame queue is empty");
	return nullptr;
}

void ISource::changeThreadStatus(bool threadWantedAlive)
{
	if (threadWantedAlive && !doNotLoadThread) {
		shouldTerminate = false;
		pthread_create(&thread, NULL, sourceThreadStart, this);
	} else if (!doNotLoadThread) {
		if (thread) {
			shouldTerminate = true;
			pthread_join(thread, NULL); // Wait for the thread to terminate
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
