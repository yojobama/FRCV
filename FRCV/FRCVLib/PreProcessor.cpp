#include "PreProcessor.h"
#include "Frame.h"
#include <opencv2/opencv.hpp>

PreProcessor::PreProcessor(FramePool* framePool)
{
	this->framePool = framePool;
}

PreProcessor::~PreProcessor()
{
	// Cleanup resources if needed
}

std::shared_ptr<Frame> PreProcessor::transformFrame(std::shared_ptr<Frame> src, FrameSpec spec)
{
	// Check if transformation is necessary
	if (src->IsIdentical(spec)) {
		return src;
	}

	bool sameHeight = (src->GetSpec().GetHeight() == spec.GetHeight());
	bool sameWidth = (src->GetSpec().GetWidth() == spec.GetWidth());
	bool sameType = (src->GetSpec().GetType() == spec.GetType());

	// If all properties match, return the source frame
	if (sameHeight && sameWidth && sameType) {
		return src;
	}

	// Create an intermediate frame with the desired type
	FrameSpec midSpec = spec;
	midSpec.SetType(CV_8UC3);
	std::shared_ptr<Frame> mid = framePool->GetFrame(midSpec);

	// Resize if dimensions differ
	if (!sameHeight || !sameWidth) {
		cv::resize(*src, *mid, cv::Size(spec.GetWidth(), spec.GetHeight()));
	} else {
		// Copy data if only the type is different
		src->copyTo(*mid);
	}

	// Create the final frame with the desired spec
	std::shared_ptr<Frame> dst = framePool->GetFrame(spec);

	// Convert type if needed
	if (!sameType) {
		int srcType = src->GetSpec().GetType();
		int dstType = spec.GetType();
		if ((srcType == CV_8UC3 && dstType == CV_8UC1) || (srcType == CV_8UC1 && dstType == CV_8UC3)) {
			int code = (srcType == CV_8UC3) ? cv::COLOR_BGR2GRAY : cv::COLOR_GRAY2BGR;
			cv::cvtColor(*mid, *dst, code);
		}
	}

	// Return the intermediate frame to the pool
	framePool->ReturnFrame(mid);

	return dst;
}
