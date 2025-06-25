#include "CameraSource.h"
#include "Frame.h"
#include <opencv2/opencv.hpp>

CameraFrameSource::CameraFrameSource(std::string devicePath, Logger* logger, FramePool* framePool) : ISource(framePool, logger)
{
    capture = new cv::VideoCapture("v4l2://" + devicePath, cv::CAP_V4L2);
    while (!capture->isOpened());
    this->devicePath = devicePath;
    this->deviceName = getDeviceName();
}

CameraFrameSource::CameraFrameSource(std::string devicePath, std::string deviceName, Logger* logger, FramePool* framePool) : ISource(framePool, logger)
{
	capture = new cv::VideoCapture(devicePath, cv::CAP_V4L2);
	this->deviceName = deviceName;
	this->devicePath = devicePath;
}

CameraFrameSource::~CameraFrameSource()
{
    delete capture;
}

std::string CameraFrameSource::getDevicePath()
{
    return devicePath;
}

std::string CameraFrameSource::getDeviceName()
{
    return deviceName;
}

void CameraFrameSource::changeDeviceName(std::string newName)
{
    this->deviceName = newName;
}

void CameraFrameSource::captureFrame()
{
    if (capture->isOpened()) {
        logger->enterLog(INFO, "camera is open, grabbing frame and returning it");
        Frame* frame = framePool->getFrame(frameSpec);
		capture->grab();
        capture->read(*frame);
        frames.push(frame);
    }
    logger->enterLog(ERROR, "camera is closed, returning a null pointer");
}
