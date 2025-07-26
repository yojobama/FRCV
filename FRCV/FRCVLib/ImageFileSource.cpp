#include "ImageFileSource.h"
#include <opencv2/opencv.hpp>
#include "Frame.h"

ImageFileFrameSource::ImageFileFrameSource(std::string filePath, Logger* logger, FramePool* framePool) : ISource(framePool, logger) {
    cv::Mat img = cv::imread(filePath);
    if (!img.empty()) {
        FrameSpec spec(img.rows, img.cols, img.type());
        frame = framePool->GetFrame(spec); // Use FramePool to allocate the frame
        img.copyTo(*frame.get());
    } else {
        frame = nullptr;    
        logger->EnterLog(LogLevel::Error, "Failed to load image from file: " + filePath);
    }
    m_DoNotLoadThread = true;
}

std::shared_ptr<Frame> ImageFileFrameSource::GetLatestFrame()
{
    if (!frame) {
        m_Logger->EnterLog(LogLevel::Error, "No frame available in ImageFileFrameSource");
    }
    return frame;
}

void ImageFileFrameSource::CaptureFrame()
{
    // No operation needed as the image is already loaded
}