#include "FramePool.h"

// initializing the frame pool
FramePool::FramePool(std::vector<FrameSpec> initialSpecs) {
	// pushing new frames to the vector in preperation for the start of the thingy
	for (FrameSpec spec : initialSpecs) {
		frameVector.push_back(new Frame(spec));
	}
}

FramePool::~FramePool() {
	delete &frameVector;
}

Frame* FramePool::getFrame(FrameSpec frameSpec) {
	for (Frame* frame : frameVector) {
		if (frame->isIdentical(frameSpec)) {
			return frame;
		}
	}
	return allocateFrame(frameSpec);
}

// allocation new frames upon request
Frame* FramePool::allocateFrame(FrameSpec frameSpec) {
	Frame* frame = new Frame(frameSpec);
	frameVector.push_back(frame);
	return frame;
}

void FramePool::returnFrame(Frame* frame) {
	frameVector.push_back(frame);
}