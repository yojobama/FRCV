#pragma once
#include <opencv2/opencv.hpp>
#include "FrameSpec.h"

class Frame : public cv::Mat
{
public:
	Frame(FrameSpec spec);
	~Frame();
	bool isIdentical(FrameSpec frameSpec);
private:
	FrameSpec spec;
};

