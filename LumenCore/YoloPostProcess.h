#pragma once
#include "IDetectionBackend.h"
#include <opencv2/opencv.hpp>
#include <vector>

// Backend-independent pre/post-processing shared by every YOLOv8/v11 execution backend
// (RKNN, ONNX Runtime, and whatever else lands here later), so the letterbox/decode/NMS math
// is written and tested exactly once rather than duplicated per backend.
namespace YoloPostProcess {

	// scale/pad bookkeeping needed to map decoded boxes back to the original image
	struct LetterboxInfo {
		float scale;
		int padLeft;
		int padTop;
		int originalWidth;
		int originalHeight;
	};

	// resizes+pads bgrFrame to exactly targetWidth x targetHeight (YOLO convention: 114,114,114
	// fill), preserving aspect ratio, and returns the bookkeeping needed to undo it afterwards
	cv::Mat Letterbox(const cv::Mat& bgrFrame, int targetWidth, int targetHeight, LetterboxInfo& outInfo);

	// Decodes one anchor-free YOLOv8/11 head output of shape [4 + numClasses, numAnchors]
	// (batch dimension already stripped by the caller) into detections in the ORIGINAL image's
	// pixel space. `outputData` must be laid out channel-major (all numAnchors values for
	// channel 0, then all for channel 1, ...), matching what `ultralytics export format=onnx`
	// produces - this does not handle a raw per-bin DFL distribution output from a non-standard
	// export; if a model needs that, it must be decoded before reaching this function.
	std::vector<ObjectDetection> DecodeAndNms(
		const float* outputData,
		int numClasses,
		int numAnchors,
		const std::vector<std::string>& labels,
		const LetterboxInfo& letterbox,
		float confThreshold,
		float nmsThreshold);

}
