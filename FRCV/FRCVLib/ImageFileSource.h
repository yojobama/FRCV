#pragma once

#include "ISource.h"
#include <string>
#include "FrameSpec.h"

class ImageFileFrameSource : public ISource
{
public:
	ImageFileFrameSource(std::string filePath, Logger* logger, FramePool* framePool);
	std::shared_ptr<Frame> getLatestFrame() override;
private:
	void captureFrame() override;
	std::shared_ptr<Frame> frame;
};

