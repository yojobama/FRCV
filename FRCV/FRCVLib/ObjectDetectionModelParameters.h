#pragma once
#include <string>

class ObjectDetectionModelParameters
{
public:
		ObjectDetectionModelParameters() = default;
		~ObjectDetectionModelParameters() = default;
		std::string modelPath;
		std::string modelType; // e.g., "YOLOv5", "SSD", etc.
		float confidenceThreshold = 0.5f; // Default confidence threshold
		float nmsThreshold = 0.4f; // Default Non-Maximum Suppression threshold
		int inputWidth = 640; // Default input width
		int inputHeight = 640; // Default input height
		int batchSize = 1; // Default batch size
		bool useGPU = false; // Whether to use GPU for inference
};

