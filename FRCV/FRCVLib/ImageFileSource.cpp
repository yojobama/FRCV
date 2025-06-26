#include "ImageFileSource.h"
#include <opencv2/opencv.hpp>
#include "Frame.h"

ImageFileFrameSource::ImageFileFrameSource(std::string filePath, Logger* logger, FramePool* framePool) : ISource(framePool, logger) {
    cv::Mat img = cv::imread(filePath);
    if (!img.empty()) {
        FrameSpec spec(img.rows, img.cols, img.type());
        frame = framePool->getFrame(spec); // Use FramePool to allocate the frame
        img.copyTo(*frame.get());
    } else {
        frame = nullptr;    
        logger->enterLog(ERROR, "Failed to load image from file: " + filePath);
    }
    doNotLoadThread = true;
}

std::shared_ptr<Frame> ImageFileFrameSource::getLatestFrame()
{
    if (!frame) {
        logger->enterLog(ERROR, "No frame available in ImageFileFrameSource");
    }
    return frame;
}

void ImageFileFrameSource::captureFrame()
{
    // No operation needed as the image is already loaded
}