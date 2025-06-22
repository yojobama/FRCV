#include "FramePool.h"
#include "Frame.h"

// initializing the frame pool
FramePool::FramePool(Logger* logger) : logger(logger) {
    if (logger) logger->enterLog("FramePool constructed");
}

FramePool::~FramePool() {
    if (logger) logger->enterLog("FramePool destructed");
}

int FramePool::getCachedFrameCount() {
    if (logger) logger->enterLog("FramePool::getCachedFrameCount called");
    return frameVector.size();
}

Frame* FramePool::getFrame(FrameSpec frameSpec) {
    if (logger) logger->enterLog("FramePool::getFrame called");
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
    if (logger) logger->enterLog("FramePool::allocateFrame called");
    logger->enterLog(INFO, "allocating a new frame");
    Frame* frame = new Frame(frameSpec);
    frameVector.push_back(frame);
    return frame;
}

void FramePool::returnFrame(Frame* frame) {
    if (logger) logger->enterLog("FramePool::returnFrame called");
    logger->enterLog(INFO, "adding an existing frame to the pool");
    frameVector.push_back(frame);
}