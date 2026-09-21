#pragma once
#include "ObjectDetection.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

// Both YOLOv8 and YOLOv11 export (via `ultralytics export format=onnx`) the same anchor-free
// head shape: [1, 4+numClasses, numAnchors], box already decoded to (cx, cy, w, h) in the
// model's input pixel space, no separate objectness channel. The two are functionally
// interchangeable to this decoder; the enum exists because it is the thing a user actually
// selects (in the model upload dropdown) and is preserved through the manifest for validation
// and in case a real head difference needs handling later, not because the math differs today.
//
// Deliberately a plain (unscoped) enum, not `enum class`: SWIG (verified with the version this
// project uses) wraps a plain C++ enum as a real C# enum, but silently falls back to an opaque
// SWIGTYPE_p_* handle for a scoped enum passed by value - which then fails at the P/Invoke
// boundary. ObjectDetectionProvider in Manager.h already relies on the same plain-enum behavior.
enum YoloVariant {
	YOLOv8,
	YOLOv11
};

struct DetectionBackendConfig {
	std::string modelPath;
	std::string labelsPath; // one class name per line, in class-index order
	YoloVariant variant = YOLOv8;
	float confThreshold = 0.25f;
	float nmsThreshold = 0.45f;
	int inputWidth = 640;
	int inputHeight = 640;
};

class IDetectionBackend {
public:
	virtual ~IDetectionBackend() = default;
	virtual bool Load(const DetectionBackendConfig& config) = 0;
	virtual std::vector<ObjectDetection> Infer(const cv::Mat& bgrFrame) = 0;
	virtual std::string Name() const = 0;
};
