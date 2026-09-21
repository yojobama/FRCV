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

	// The shared tail of every decoder here: run NMS over candidate boxes (already in letterboxed-
	// frame pixel space) and map survivors back into the ORIGINAL frame's pixel coordinates.
	// Factored out of DecodeAndNms so DecodeDflMultiScaleAndNms (a differently-shaped decode -
	// see its own comment) doesn't have to duplicate the NMS/unmapping/labelling logic.
	std::vector<ObjectDetection> NmsAndBuildDetections(
		const std::vector<cv::Rect>& boxesForNms,
		const std::vector<float>& scores,
		const std::vector<int>& classIds,
		const std::vector<std::string>& labels,
		const LetterboxInfo& letterbox,
		float confThreshold,
		float nmsThreshold);

	// One FPN scale's raw box/class tensors from an un-fused RKNN YOLOv8 export (the
	// airockchip/rknn_model_zoo export recipe - see RknnDetectionBackend's own comment for why
	// this differs from the single-fused-head shape ultralytics' ONNX export produces). Both
	// tensors are already dequantized to float32 (via rknn_output::want_float=1) and laid out
	// NCHW with the batch dimension stripped: boxData is [4*regMax, gridH, gridW], clsData is
	// [numClasses, gridH, gridW].
	struct DflScaleOutput {
		const float* boxData;
		const float* clsData;
		int gridH, gridW;
		int stride; // input pixels per grid cell at this scale (e.g. 8/16/32 for a 640 input)
	};

	// Decodes YOLOv8's anchor-free DFL (distribution focal loss) box regression - a per-side
	// (left/top/right/bottom) discrete probability distribution over regMax bins, integrated to a
	// continuous distance - plus a per-class sigmoid score, across every FPN scale, then NMS's the
	// combined candidate set. This is NOT the same math as DecodeAndNms's box decode (which
	// assumes boxes already regressed to cx/cy/w/h by the export graph itself); RKNN's un-fused
	// export leaves the DFL distribution undecoded specifically so the NPU graph can skip the
	// integral/argmax work, deferring it to this CPU-side post-process instead.
	std::vector<ObjectDetection> DecodeDflMultiScaleAndNms(
		const std::vector<DflScaleOutput>& scales,
		int regMax,
		int numClasses,
		const std::vector<std::string>& labels,
		const LetterboxInfo& letterbox,
		float confThreshold,
		float nmsThreshold);

}
