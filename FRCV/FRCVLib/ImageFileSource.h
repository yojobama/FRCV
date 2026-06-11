#pragma once

#include "ISource.h"
#include <string>
#include "FrameSpec.h"

class ImageFileFrameSource : public ISource
{
public:
	ImageFileFrameSource(std::string filePath, std::shared_ptr<Logger> logger, std::string m_ID);
private:
	void CaptureFrame() override;
	cv::Mat mat;
};

