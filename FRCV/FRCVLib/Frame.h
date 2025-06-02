#pragma once
#include <opencv2/opencv.hpp>
#include "FrameSpec.h"

class Frame : cv::Mat
{
public:
	Frame();
	~Frame();
	bool isIdentical(FrameSpec frameSpec);
};

