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
	if (src->isIdentical(spec)) {
		return src;
	}

	bool sameHeight = (src->getSpec().getHeight() == spec.getHeight());
	bool sameWidth = (src->getSpec().getWidth() == spec.getWidth());
	bool sameType = (src->getSpec().getType() == spec.getType());

	// If all properties match, return the source frame
	if (sameHeight && sameWidth && sameType) {
		return src;
	}

	// Create an intermediate frame with the desired type
	FrameSpec midSpec = spec;
	midSpec.setType(CV_8UC3);
	std::shared_ptr<Frame> mid = framePool->getFrame(midSpec);

	// Resize if dimensions differ
	if (!sameHeight || !sameWidth) {
		cv::resize(*src, *mid, cv::Size(spec.getWidth(), spec.getHeight()));
	} else {
		// Copy data if only the type is different
		src->copyTo(*mid);
	}

	// Create the final frame with the desired spec
	std::shared_ptr<Frame> dst = framePool->getFrame(spec);

	// Convert type if needed
	if (!sameType) {
		int srcType = src->getSpec().getType();
		int dstType = spec.getType();
		if ((srcType == CV_8UC3 && dstType == CV_8UC1) || (srcType == CV_8UC1 && dstType == CV_8UC3)) {
			int code = (srcType == CV_8UC3) ? cv::COLOR_BGR2GRAY : cv::COLOR_GRAY2BGR;
			cv::cvtColor(*mid, *dst, code);
		}
	}

	// Return the intermediate frame to the pool
	framePool->returnFrame(mid);

	return dst;
}
