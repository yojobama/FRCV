#include "CameraSource.h"
#include "Frame.h"
#include <opencv2/opencv.hpp>

CameraFrameSource::CameraFrameSource(std::string devicePath, std::shared_ptr<Logger> logger, std::string id) : ISource(logger, id)
{
    capture = cv::VideoCapture(devicePath, cv::CAP_V4L2);

    if (!capture.isOpened()) throw "unable to open webcam: " + devicePath;

    this->m_DevicePath = devicePath;
    this->m_DeviceName = getDeviceName();
}

CameraFrameSource::CameraFrameSource(std::string devicePath, std::string deviceName, std::shared_ptr<Logger> logger, std::string id) : ISource(logger, id)
{
	capture = cv::VideoCapture(devicePath, cv::CAP_V4L2);
	this->m_DeviceName = deviceName;
	this->m_DevicePath = devicePath;
}

CameraFrameSource::~CameraFrameSource()
{
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
    if (capture.isOpened()) {
        cv::Mat mat;
        capture >> mat;
		SetLatestResult(SourceResult(std::nullopt, mat));
     }
    m_Logger->EnterLog(LogLevel::Error, "camera is closed, not capturing a frame");
}
