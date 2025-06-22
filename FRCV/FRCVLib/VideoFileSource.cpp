#include "VideoFileSource.h"
#include "ImageFileSource.h"
#include "Frame.h"

VideoFileFrameSource::VideoFileFrameSource(Logger* logger, std::string filePath, FramePool* framePool) : ISource(framePool, logger)
{
	this->logger = logger;
	if (logger) logger->enterLog("VideoFileFrameSource constructed with filePath: " + filePath);
	logger->enterLog(INFO, "initializing a video file capture device");
	this->capture = new cv::VideoCapture(filePath);
}

Frame* VideoFileFrameSource::getFrame()
{
    lock.lock();
    if (logger) logger->enterLog("VideoFileFrameSource::getFrame called");
    Frame* frame = framePool->getFrame(frameSpec);
    if (capture->isOpened()) {
        logger->enterLog(INFO, "camera is open, grabbing frame and returning it");
        capture->read(*frame);
        return frame;
    }
    logger->enterLog(ERROR, "camera is closed, returning a null pointer");
	lock.unlock();
    return frame;
}
