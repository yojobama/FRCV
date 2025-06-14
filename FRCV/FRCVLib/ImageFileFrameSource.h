#pragma once

#include "IFrameSource.h"
#include <opencv2/opencv.hpp>
#include <string>
#include "Frame.h"
#include "FrameSpec.h"

class ImageFileFrameSource : public IFrameSource
{
public:
	ImageFileFrameSource(std::string filePath);
	Frame* getFrame();
private:
	Frame* frame;
};

