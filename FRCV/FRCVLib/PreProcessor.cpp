#include "PreProcessor.h"
#include "Frame.h"

PreProcessor::PreProcessor(FramePool* framePool)
{
	this->framePool = framePool;
}

PreProcessor::~PreProcessor()
{
	// TODO: implement this function
}

Frame* PreProcessor::transformFrame(Frame* src, FrameSpec spec)
{
	// first check, see if enything has to be done or not
	if (src->isIdentical(spec)) {
		return src;
	}
	bool sameHeight, sameWidth, sameType;

	// check height
	sameHeight = (src->getSpec().getHeight() == spec.getHeight());
	
	// check width
	sameWidth = (src->getSpec().getWidth() == spec.getWidth());

	// check frame type (RGB, BGR, gray scale, etc...)
	sameType = (src->getSpec().getType() == spec.getType());

	// If all are the same, return src (should not reach here due to isIdentical, but for safety)
	if (sameHeight && sameWidth && sameType) {
		return src;
	}

	// Otherwise, create a new frame with the desired spec
	Frame* dst = framePool->getFrame(spec);

	// Resize if needed
	if (!sameHeight || !sameWidth) {
		cv::resize(*src, *dst, cv::Size(spec.getWidth(), spec.getHeight()));
	} else {
		// Copy data if only type is different
		src->copyTo(*dst);
	}

	// Convert type if needed
	if (!sameType) {
		int srcType = src->getSpec().getType();
		int dstType = spec.getType();
		// Example: convert between color spaces (assuming OpenCV types)
		if ((srcType == CV_8UC3 && dstType == CV_8UC1) || (srcType == CV_8UC1 && dstType == CV_8UC3)) {
			int code = (srcType == CV_8UC3) ? cv::COLOR_BGR2GRAY : cv::COLOR_GRAY2BGR;
			cv::cvtColor(*dst, *dst, code);
		}
		// Add more conversions as needed
	}

	// return src to the frame pool
	framePool->returnFrame(src);

	return dst;
}
