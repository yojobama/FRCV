#pragma once
#include "IObjectDetectionSink.h"

class ONNXSink : public IObjectDetectionSink
{
public:
	// TODO: Add parameters for ONNX model path, input size, etc.
	// in short, implement the bloody thing
	ONNXSink(std::string onnxREP, std::string modelPath, Logger* logger, PreProcessor* preProcessor, FramePool* framePool);
	~ONNXSink();
	std::string GetStatus() override;
};

