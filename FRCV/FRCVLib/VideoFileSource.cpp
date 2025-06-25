#include "VideoFileSource.h"
#include "ImageFileSource.h"
#include "Frame.h"
#include <chrono>
#include <pthread.h>

VideoFileFrameSource::VideoFileFrameSource(Logger* logger, std::string filePath, FramePool* framePool, int fps) : ISource(framePool, logger)
{
	this->logger = logger;
	if (logger) logger->enterLog("VideoFileFrameSource constructed with filePath: " + filePath);
	logger->enterLog(INFO, "initializing a video file capture device");
	this->capture = new cv::VideoCapture(filePath);
    this->fps = fps;
}

void VideoFileFrameSource::captureFrame()
{
    if (capture->isOpened()) {
        logger->enterLog(INFO, "camera is open, grabbing frame and returning it");
        Frame* frame = framePool->getFrame(frameSpec);
        capture->grab();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
        capture->read(*frame);
        frames.push(frame);
    }
    logger->enterLog(ERROR, "camera is closed, returning a null pointer");
}
