#include "VideoFileFrameSource.h"
#include "ImageFileFrameSource.h"

VideoFileFrameSource::VideoFileFrameSource(Logger* logger, std::string filePath)
{
	this->logger = logger;
	logger->enterLog(INFO, "initializing a video file capture device");
	this->capture = new cv::VideoCapture(filePath);
}

Frame* VideoFileFrameSource::getFrame()
{
	return nullptr;
}
