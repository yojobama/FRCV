#include "FramePool.h"

// initializing the frame pool
FramePool::FramePool(Logger* logger) {
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