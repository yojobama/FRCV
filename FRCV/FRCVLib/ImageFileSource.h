#pragma once

#include "ISource.h"
#include <string>
#include "FrameSpec.h"

class ImageFileFrameSource : public ISource
{
public:
	ImageFileFrameSource(std::string filePath, Logger* logger, std::string m_ID);
private:
	void CaptureFrame() override;
	cv::Mat mat;
};

