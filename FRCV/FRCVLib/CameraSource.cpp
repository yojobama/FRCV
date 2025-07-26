#include "CameraSource.h"
#include "Frame.h"
#include <opencv2/opencv.hpp>

CameraFrameSource::CameraFrameSource(std::string devicePath, Logger* logger, FramePool* framePool) : ISource(framePool, logger)
{
    capture = new cv::VideoCapture(devicePath, cv::CAP_V4L2);

    if (!capture->isOpened()) throw "unable to open webcam: " + devicePath;

    this->m_DevicePath = devicePath;
    this->m_DeviceName = getDeviceName();
}

CameraFrameSource::CameraFrameSource(std::string devicePath, std::string deviceName, Logger* logger, FramePool* framePool) : ISource(framePool, logger)
{
	capture = new cv::VideoCapture(devicePath, cv::CAP_V4L2);
	this->m_DeviceName = deviceName;
	this->m_DevicePath = devicePath;
}

CameraFrameSource::~CameraFrameSource()
{
    delete capture;
}

std::string CameraFrameSource::getDevicePath()
{
    return m_DevicePath;
}

std::string CameraFrameSource::getDeviceName()
{
    return m_DeviceName;
}

void CameraFrameSource::changeDeviceName(std::string newName)
{
    this->m_DeviceName = newName;
}

void CameraFrameSource::CaptureFrame()
{
    if (capture->isOpened()) {
        m_Logger->EnterLog(LogLevel::Info, "camera is open, grabbing frame and returning it");
        std::shared_ptr<Frame> frame = m_FramePool->GetFrame(m_FrameSpec);
        capture->read(*frame);
        m_Frames.push(frame);
    }
    m_Logger->EnterLog(LogLevel::Error, "camera is closed, returning a null pointer");
}
