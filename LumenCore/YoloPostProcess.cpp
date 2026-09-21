#include "YoloPostProcess.h"
#include <opencv2/dnn.hpp>
#include <algorithm>

namespace YoloPostProcess {

cv::Mat Letterbox(const cv::Mat& bgrFrame, int targetWidth, int targetHeight, LetterboxInfo& outInfo)
{
	outInfo.originalWidth = bgrFrame.cols;
	outInfo.originalHeight = bgrFrame.rows;

	float scale = std::min(
		static_cast<float>(targetWidth) / bgrFrame.cols,
		static_cast<float>(targetHeight) / bgrFrame.rows);
	outInfo.scale = scale;

	int unpaddedWidth = static_cast<int>(std::round(bgrFrame.cols * scale));
	int unpaddedHeight = static_cast<int>(std::round(bgrFrame.rows * scale));

	cv::Mat resized;
	cv::resize(bgrFrame, resized, cv::Size(unpaddedWidth, unpaddedHeight));

	outInfo.padLeft = (targetWidth - unpaddedWidth) / 2;
	outInfo.padTop = (targetHeight - unpaddedHeight) / 2;
	int padRight = targetWidth - unpaddedWidth - outInfo.padLeft;
	int padBottom = targetHeight - unpaddedHeight - outInfo.padTop;

	cv::Mat padded;
	cv::copyMakeBorder(resized, padded, outInfo.padTop, padBottom, outInfo.padLeft, padRight,
		cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));
	return padded;
}

std::vector<ObjectDetection> DecodeAndNms(
	const float* outputData,
	int numClasses,
	int numAnchors,
	const std::vector<std::string>& labels,
	const LetterboxInfo& letterbox,
	float confThreshold,
	float nmsThreshold)
{
	std::vector<cv::Rect> boxesForNms; // in letterboxed-frame pixel space, NMS only needs relative geometry
	std::vector<float> scores;
	std::vector<int> classIds;

	for (int anchor = 0; anchor < numAnchors; anchor++) {
		int bestClassId = -1;
		float bestScore = 0.0f;
		for (int c = 0; c < numClasses; c++) {
			float score = outputData[(4 + c) * numAnchors + anchor];
			if (score > bestScore) {
				bestScore = score;
				bestClassId = c;
			}
		}

		if (bestScore < confThreshold) continue;

		float cx = outputData[0 * numAnchors + anchor];
		float cy = outputData[1 * numAnchors + anchor];
		float w = outputData[2 * numAnchors + anchor];
		float h = outputData[3 * numAnchors + anchor];

		int left = static_cast<int>(std::round(cx - w / 2.0f));
		int top = static_cast<int>(std::round(cy - h / 2.0f));

		boxesForNms.emplace_back(left, top, static_cast<int>(std::round(w)), static_cast<int>(std::round(h)));
		scores.push_back(bestScore);
		classIds.push_back(bestClassId);
	}

	std::vector<int> keptIndices;
	cv::dnn::NMSBoxes(boxesForNms, scores, confThreshold, nmsThreshold, keptIndices);

	std::vector<ObjectDetection> detections;
	detections.reserve(keptIndices.size());

	for (int idx : keptIndices) {
		const cv::Rect& letterboxedBox = boxesForNms[idx];

		// map the box out of letterbox space back into the original frame's pixel coordinates
		double origLeft = (letterboxedBox.x - letterbox.padLeft) / letterbox.scale;
		double origTop = (letterboxedBox.y - letterbox.padTop) / letterbox.scale;
		double origWidth = letterboxedBox.width / letterbox.scale;
		double origHeight = letterboxedBox.height / letterbox.scale;

		// clamp to the original frame - a box near the letterbox padding can extend slightly
		// outside it after unscaling
		origLeft = std::clamp(origLeft, 0.0, static_cast<double>(letterbox.originalWidth));
		origTop = std::clamp(origTop, 0.0, static_cast<double>(letterbox.originalHeight));
		origWidth = std::min(origWidth, letterbox.originalWidth - origLeft);
		origHeight = std::min(origHeight, letterbox.originalHeight - origTop);

		ObjectDetection detection;
		detection.SetBoundingBox(cv::Rect2d(origLeft, origTop, origWidth, origHeight));
		detection.SetConfidence(scores[idx]);
		detection.SetClassId(classIds[idx]);
		detection.SetClassName(classIds[idx] < static_cast<int>(labels.size()) ? labels[classIds[idx]] : std::to_string(classIds[idx]));
		detections.push_back(detection);
	}

	return detections;
}

}
