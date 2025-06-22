#include "CameraSource.h"
#include "Frame.h"

CameraFrameSource::CameraFrameSource(std::string devicePath, Logger* logger, FramePool* framePool) : ISource(framePool, logger)
{
    capture = new cv::VideoCapture(devicePath, cv::CAP_V4L2);
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

Frame* CameraFrameSource::getFrame() {  
	lock.lock();
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