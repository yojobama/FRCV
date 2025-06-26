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
    std::lock_guard<std::mutex> guard(lock);
    return frameVector.size();
}

std::shared_ptr<Frame> FramePool::getFrame(FrameSpec frameSpec) {
    if (logger) logger->enterLog("FramePool::getFrame called");
    {
        std::lock_guard<std::mutex> guard(lock);
        for (auto it = frameVector.begin(); it != frameVector.end(); ++it) {
            if ((*it)->isIdentical(frameSpec)) {
                logger->enterLog(INFO, "retrieving an existing cached frame from the pool");
                std::shared_ptr<Frame> frame = *it;
                frameVector.erase(it); // Remove the frame from the pool
                return frame;
            }
        }
    }
    return allocateFrame(frameSpec);
}

// allocation new frames upon request
std::shared_ptr<Frame> FramePool::allocateFrame(FrameSpec frameSpec) {
    if (logger) logger->enterLog("FramePool::allocateFrame called");
    logger->enterLog(INFO, "allocating a new frame");
    std::shared_ptr<Frame> frame = std::make_shared<Frame>(frameSpec);
    {
        std::lock_guard<std::mutex> guard(lock);
        frameVector.push_back(frame);
    }
    return frame;
}

void FramePool::returnFrame(std::shared_ptr<Frame> frame) {
    if (logger) logger->enterLog("FramePool::returnFrame called");
    logger->enterLog(INFO, "adding an existing frame to the pool");
    {
        std::lock_guard<std::mutex> guard(lock);
        if (std::find(frameVector.begin(), frameVector.end(), frame) == frameVector.end()) {
            frameVector.push_back(frame);
        } else {
            logger->enterLog(INFO, "Frame is already in the pool, skipping");
        }
    }
}