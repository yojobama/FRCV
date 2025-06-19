#include "VideoFileFrameSource.h"
#include "ImageFileFrameSource.h"

VideoFileFrameSource::VideoFileFrameSource(Logger* logger, std::string filePath, FramePool* framePool) : IFrameSource(framePool, logger)
{
	this->logger = logger;
	logger->enterLog(INFO, "initializing a video file capture device");
	this->capture = new cv::VideoCapture(filePath);
}

Frame* VideoFileFrameSource::getFrame()
{
    Frame* frame = framePool->getFrame(frameSpec);
    if (capture->isOpened()) {
        logger->enterLog(INFO, "camera is open, grabbing frame and returning it");
        capture->read(*frame);
        return frame;
    }
    logger->enterLog(ERROR, "camera is closed, returning a null pointer");
    return nullptr;
}
