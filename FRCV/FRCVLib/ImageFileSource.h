#pragma once

#include "ISource.h"
#include <string>
#include "FrameSpec.h"

class ImageFileFrameSource : public ISource
{
public:
	ImageFileFrameSource(std::string filePath, Logger* logger, FramePool* framePool);
	Frame* getLatestFrame() override;
private:
	void captureFrame() override;
	Frame* frame;
};

