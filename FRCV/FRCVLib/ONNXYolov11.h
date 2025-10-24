#pragma once
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

#include <string>
#include <memory>

#include "ObjectDetectionHelpers.h"

class Logger;
class Frame;
class PreProcessor;

class ONNXYolov11
{
public:
	ONNXYolov11(Logger* logger, PreProcessor* preProcessor, std::string modelPath, std::string labelsPath, std::string onnxREP = "");
	~ONNXYolov11();

	std::vector<std::string> GetAvailableREPs();

	std::vector<Detection> Detect(std::shared_ptr<Frame> frame);

	cv::Size GetExpectedInputSize();

	void setONNXREP(std::string onnxREP);
	void setConfThreshold(float confThreshold);
	void setIouThreshold(float iouThreshold);

	float getConfThreshold();
	float getIouThreshold();
private:
	PreProcessor* m_PreProcessor;
	Logger* m_Logger;

	bool m_UsingREP;
	std::string m_ONNXREP;
	float m_ConfThreshold = 0.4f;
	float m_IouThreshold = 0.45f;

	// ONNX stuff
	Ort::Env m_Env { nullptr };
	Ort::SessionOptions m_SessionOptions { nullptr };
	Ort::Session m_Session { nullptr };
	bool m_IsDynamicInputShape;
	cv::Size m_InputSize; // expected input shape for the model

	// vectors to hold allocated input and output node names
	std::vector<Ort::AllocatedStringPtr> inputNodeNameAllocatedStrings;
	std::vector<const char*> inputNames;
	std::vector<Ort::AllocatedStringPtr> outputNodeNameAllocatedStrings;
	std::vector<const char*> outputNames;

	unsigned long numInputNodes, numOutputNodes;

	std::vector<std::string> classNames;
	std::vector<cv::Scalar> classColours;

	std::shared_ptr<Frame> preprocess(std::shared_ptr<Frame> frame, float* blob, std::vector<long> inputTensorShape);
	std::vector<Detection> postprocess(cv::Size originalImageSize, cv::Size resizedImageShape, std::vector<Ort::Value> outputTensors, float confThreshold, float iouThreshold);
};

