#pragma once

#include "IFrameSource.h"
#include <opencv2/opencv.hpp>

class ImageFileFrameSource : public IFrameSource
{
public:
	ImageFileFrameSource(std::string& filePath);
private:
	cv::Mat *frame;
};

