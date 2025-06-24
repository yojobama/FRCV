#include "ImageFileSource.h"
#include <opencv2/opencv.hpp>
#include "Frame.h"

ImageFileFrameSource::ImageFileFrameSource(std::string filePath, Logger* logger, FramePool* framePool) : ISource(framePool, logger) {
    cv::Mat img = cv::imread(filePath);
    if (!img.empty()) {
        FrameSpec spec(img.rows, img.cols, img.type());
        frame = new Frame(spec);
        img.copyTo(*frame);
        frames.push(frame);
    }
    else {
        frame = nullptr;
    }
}

void ImageFileFrameSource::captureFrame()
{
    // there is nothing to do as the image is already loaded...
}