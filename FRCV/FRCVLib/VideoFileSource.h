#pragma once
#include "IFrameSource.h"
#include <string>
#include "Logger.h"

class VideoFileFrameSource : public ISource
{
public:
	VideoFileFrameSource(Logger* logger, std::string filePath, FramePool* framePool);
	Frame* getFrame();
private:
	Logger* logger;
	cv::VideoCapture* capture;
};

