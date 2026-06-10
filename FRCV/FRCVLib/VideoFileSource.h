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
	VideoFileFrameSource(Logger* logger, std::string filePath, int fps, std::string m_ID);
private:
	void CaptureFrame() override;
	Logger* m_Logger;
	cv::VideoCapture m_Capture;
	int m_Fps;
};

