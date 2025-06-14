#include "FramePool.h"

// initializing the frame pool
FramePool::FramePool(std::vector<FrameSpec> initialSpecs, Logger* logger) {
	// pushing new frames to the vector in preperation for the start of the thingy
	for (FrameSpec spec : initialSpecs) {
		frameVector.push_back(new Frame(spec));
	}
	this->logger = logger;
}

FramePool::~FramePool() {
	delete &frameVector;
}

int FramePool::getCachedFrameCount()
{
	return frameVector.size();
}

Frame* FramePool::getFrame(FrameSpec frameSpec) {
	for (Frame* frame : frameVector) {
		if (frame->isIdentical(frameSpec)) {
			logger->enterLog(INFO, "retriving an existing cached frame from the pool");
			return frame;
		}
	}
	return allocateFrame(frameSpec);
}

// allocation new frames upon request
Frame* FramePool::allocateFrame(FrameSpec frameSpec) {
	logger->enterLog(INFO, "allocating a new frame");
	Frame* frame = new Frame(frameSpec);
	frameVector.push_back(frame);
	return frame;
}

void FramePool::returnFrame(Frame* frame) {
	logger->enterLog(INFO, "adding an existing frame to the pool");
	frameVector.push_back(frame);
}