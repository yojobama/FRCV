#pragma once

#include "ISource.h"
#include <string>
#include "FrameSpec.h"

class ImageFileFrameSource : public ISource
{
public:
	ImageFileFrameSource(std::string filePath, Logger* logger, FramePool* framePool);
	std::shared_ptr<Frame> GetLatestFrame() override;
private:
	void CaptureFrame() override;
	std::shared_ptr<Frame> frame;
};

