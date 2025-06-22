#pragma once
#include "ISource.h"
#include <string>
#include "Logger.h"

namespace cv {
	class VideoCapture;
}

class VideoFileFrameSource : public ISource
{
public:
	VideoFileFrameSource(Logger* logger, std::string filePath, FramePool* framePool);
	Frame* getFrame();
private:
	Logger* logger;
	cv::VideoCapture* capture;
};

