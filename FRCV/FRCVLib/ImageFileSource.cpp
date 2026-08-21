#include "ImageFileSource.h"
#include <opencv2/opencv.hpp>

ImageFileFrameSource::ImageFileFrameSource(std::string filePath, std::shared_ptr<Logger> logger, std::string m_ID) : ISource(logger, m_ID) {
    cv::Mat img = cv::imread(filePath);
    if (!img.empty()) {
        mat = img;
        // a static image only ever changes once, at load - there is no capture thread (see
        // m_DoNotLoadCaptureThread below) to call SetLatestResult() later the way every other
        // source type does, so it must happen here or a bound sink never sees anything to
        // process at all (confirmed by actually running the pipeline: it silently never called
        // Process(), since ISink::ProcessingThreadLoop only acts once a source's frame count
        // changes, which SetLatestResult() is what bumps).
        SetLatestResult(SourceResult(std::nullopt, mat));
    } else {
        mat = cv::Mat();
        logger->EnterLog(LogLevel::Error, "Failed to load image from file: " + filePath);
    }
    m_DoNotLoadCaptureThread = true;
}

void ImageFileFrameSource::CaptureFrame()
{
    // No operation needed as the image is already loaded
}