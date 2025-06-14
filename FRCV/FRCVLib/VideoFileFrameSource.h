#pragma once
#include "IFrameSource.h"
#include <string>
#include "Logger.h"

class VideoFileFrameSource : public IFrameSource
{
public:
	VideoFileFrameSource(Logger* logger, std::string filePath);
	Frame* getFrame();
private:
	Logger* logger;
	cv::VideoCapture* capture;
};

