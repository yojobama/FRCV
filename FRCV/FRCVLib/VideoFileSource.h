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
	VideoFileFrameSource(Logger* logger, std::string filePath, FramePool* framePool, int fps);
	Frame* getFrame();
private:
	void captureFrame() override;
	Logger* logger;
	cv::VideoCapture* capture;
	int fps;
};

