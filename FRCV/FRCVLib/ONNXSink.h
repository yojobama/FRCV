#pragma once
#include "IObjectDetectionSink.h"

class ONNXSink : public IObjectDetectionSink
{
public:
	// TODO: Add parameters for ONNX model path, input size, etc.
	// in short, implement the bloody thing
	// need to add a way to change (and even set in the first place) the ONNX REP
	ONNXSink(std::string onnxREP, ObjectDetectionModelParameters modelParameters, Logger* logger, PreProcessor* preProcessor, FramePool* framePool);
	~ONNXSink();
	std::string GetStatus() override;
private:
	std::string m_ONNXREP;
	void ProcessFrame() override;
};

