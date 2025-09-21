#include "VideoFileSource.h"
#include "ImageFileSource.h"
#include "Frame.h"
#include <chrono>
#include <thread>
#include <pthread.h>

VideoFileFrameSource::VideoFileFrameSource(Logger* logger, std::string filePath, FramePool* framePool, int fps) : ISource(framePool, logger)
{
	this->m_Logger = logger;
	if (logger) logger->EnterLog("VideoFileFrameSource constructed with filePath: " + filePath);
	logger->EnterLog(LogLevel::Info, "initializing a video file capture device");
	this->m_Capture = new cv::VideoCapture(filePath);
	if (!m_Capture->isOpened()) throw "unable to open video file: " + filePath;
    this->m_Fps = fps;
}

void VideoFileFrameSource::CaptureFrame()
{
    if (m_Capture->isOpened()) {
        m_Logger->EnterLog(LogLevel::Info, "camera is open, grabbing frame and returning it");
        std::shared_ptr<Frame> frame = m_FramePool->GetFrame(m_FrameSpec);
        m_Capture->grab();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / m_Fps));
        if (m_Capture->read(*frame.get())) {
            m_FrameCount++;
            frame.get()->SetFrameNumber(m_FrameCount);
            m_Frames.push(frame);
        } else {
            m_Logger->EnterLog(LogLevel::Error, "Failed to read frame from video file");
            m_FramePool->ReturnFrame(frame); // Return unused frame to the pool
        }
    } else {
        m_Logger->EnterLog(LogLevel::Error, "camera is closed, cannot capture frame");
    }
}
