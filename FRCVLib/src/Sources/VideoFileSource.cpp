#include "../VideoFileSource.h"
#include "ImageFileSource.h"
#include "../Frame.h"
#include <chrono>
#include <pthread.h>

VideoFileFrameSource::VideoFileFrameSource(Logger* logger, std::string filePath, FramePool* framePool, int fps) : ISource(framePool, logger)
{
	this->logger = logger;
	if (logger) logger->enterLog("VideoFileFrameSource constructed with filePath: " + filePath);
	logger->enterLog(INFO, "initializing a video file capture device");
	this->capture = new cv::VideoCapture(filePath);
	if (!capture->isOpened()) throw "unable to open video file: " + filePath;
    this->fps = fps;
}

void VideoFileFrameSource::captureFrame()
{
    if (capture->isOpened()) {
        logger->enterLog(INFO, "camera is open, grabbing frame and returning it");
        std::shared_ptr<Frame> frame = framePool->getFrame(frameSpec);
        capture->grab();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
        if (capture->read(*frame)) {
            frames.push(frame);
        } else {
            logger->enterLog(ERROR, "Failed to read frame from video file");
            framePool->returnFrame(frame); // Return unused frame to the pool
        }
    } else {
        logger->enterLog(ERROR, "camera is closed, cannot capture frame");
    }
}
