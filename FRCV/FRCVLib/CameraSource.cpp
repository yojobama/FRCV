#include "CameraSource.h"
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
        // stamped the instant the read returns, not when SetLatestResult later publishes it -
        // the two can be an arbitrary amount of time apart once the sink processing thread is
        // busy, and stereo pairing needs the real capture instant to gate on skew correctly
        uint64_t captureTimeUs = SourceResult::NowUs();
		SetLatestResult(SourceResult(std::nullopt, mat, captureTimeUs));
     }
    m_Logger->EnterLog(LogLevel::Error, "camera is closed, not capturing a frame");
}
