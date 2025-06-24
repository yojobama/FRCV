#pragma once
#include <opencv2/opencv.hpp>
#include "FrameSpec.h"

class FramePool;

class Frame : public cv::Mat
{
public:
    Frame() : spec(0, 0, 0) {} // Default constructor
	Frame(FrameSpec spec);

	~Frame(); // TODO: implement the Frame classe's destructor

	int getReferences();
	void reference();
	void dereference();

	bool isIdentical(FrameSpec frameSpec);
	FrameSpec getSpec();
private:
	int referenceCount;
	FrameSpec spec;
};
