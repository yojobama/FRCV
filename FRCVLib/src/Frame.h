#pragma once
#include <opencv2/opencv.hpp>
#include "FrameSpec.h"

class FramePool;

class Frame : public cv::Mat
{
public:
    Frame() : spec(0, 0, 0) {} // Default constructor
	Frame(FrameSpec spec);

	~Frame() = default; // TODO: implement the Frame classe's destructor

	bool isIdentical(FrameSpec frameSpec);
	FrameSpec getSpec();
private:
	FrameSpec spec;
};
