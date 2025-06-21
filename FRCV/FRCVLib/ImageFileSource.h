#pragma once

#include "ISource.h"
#include <opencv2/opencv.hpp>
#include <string>
#include "Frame.h"
#include "FrameSpec.h"

class ImageFileFrameSource : public ISource
{
public:
	ImageFileFrameSource(std::string filePath, Logger* logger);
	Frame* getFrame();
private:
	Frame* frame;
};

