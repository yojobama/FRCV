#include "ONNXYolov11.h"

ONNXYolov11::ONNXYolov11(std::string modelPath, std::string labelsPath, std::string onnxREP)
{
}

ONNXYolov11::~ONNXYolov11()
{
}

std::vector<Detection> ONNXYolov11::detect(std::shared_ptr<Frame> frame)
{
	return std::vector<Detection>();
}

void ONNXYolov11::setONNXREP(std::string onnxREP)
{
}

void ONNXYolov11::setConfThreshold(float confThreshold)
{
}

void ONNXYolov11::setIouThreshold(float iouThreshold)
{
}

float ONNXYolov11::getConfThreshold()
{
	return 0.0f;
}

float ONNXYolov11::getIouThreshold()
{
	return 0.0f;
}

std::shared_ptr<Frame> ONNXYolov11::preprocess(std::shared_ptr<Frame> frame, float* blob, std::vector<long> inputTensorShape)
{
	return std::shared_ptr<Frame>();
}

std::vector<Detection> ONNXYolov11::postprocess(cv::Size originalImageSize, cv::Size resizedImageShape, std::vector<Ort::Value> outputTensors, float confThreshold, float iouThreshold)
{
	return std::vector<Detection>();
}
