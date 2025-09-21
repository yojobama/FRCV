#pragma once
#include <opencv2/opencv.hpp>
#include "FrameSpec.h"

class FramePool;

class Frame : public cv::Mat
{
public:
    Frame() : m_Spec(0, 0, 0) {} // Default constructor
	Frame(FrameSpec spec);

	~Frame() = default; // TODO: implement the Frame classe's destructor

	bool IsIdentical(FrameSpec frameSpec);
	FrameSpec GetSpec();

	uint64_t GetFrameNumber();
	void SetFrameNumber(uint64_t frameNumber);
private:
	FrameSpec m_Spec;
	uint64_t m_FrameNumber;
};
