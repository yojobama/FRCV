#include "ImageFileFrameSource.h"
#include <opencv2/opencv.hpp>

ImageFileFrameSource::ImageFileFrameSource(std::string filePath, Logger* logger) : ISource(nullptr, logger) {
    cv::Mat img = cv::imread(filePath);
    if (!img.empty()) {
        FrameSpec spec(img.rows, img.cols, img.type());
        frame = new Frame(spec);
        img.copyTo(*frame);
    }
    else {
        frame = nullptr;
    }
}

Frame* ImageFileFrameSource::getFrame() {
    return frame;
}