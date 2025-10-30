#include "ONNXYolov11.h"
#include "Frame.h"
#include "Logger.h"
#include "ObjectDetectionHelpers.h"
#include "PreProcessor.h"

#include <thread>

ONNXYolov11::ONNXYolov11(Logger* logger, PreProcessor* preProcessor, std::string modelPath, std::string labelsPath, std::string onnxREP)
{
	m_PreProcessor = preProcessor;
	m_Logger = logger;

	m_Env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "ONNX_YOLO11_DETECTION");
	m_SessionOptions = Ort::SessionOptions();

	m_SessionOptions.SetIntraOpNumThreads(std::min(6, static_cast<int>(std::thread::hardware_concurrency())));
	m_SessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
	
	if (onnxREP != "")
	{
		m_Logger->EnterLog(LogLevel::Info, "ONNXYolov11: Setting ONNX Runtime Execution Provider to " + onnxREP);
		m_UsingREP = true;
		m_ONNXREP = onnxREP;
		// for each ERP create the required provider options and add them to the session options (cuda, rocm, openvino, etc...)
		if (onnxREP == "CUDAExecutionProvider") {
			m_SessionOptions.AppendExecutionProvider_CUDA(OrtCUDAProviderOptions());
		}
		else {
			m_Logger->EnterLog(LogLevel::Warning, "ONNXYolov11: The specified ONNX Runtime Execution Provider (" + onnxREP + ") is not supported. Falling back to default CPU Execution Provider.");
			m_UsingREP = false;
		}
	}
	else
	{
		m_Logger->EnterLog(LogLevel::Info, "ONNXYolov11: Using default ONNX Runtime CPU Execution Provider.");
		m_UsingREP = false;
	}

	m_Session = Ort::Session(m_Env, modelPath.c_str(), m_SessionOptions);

	Ort::AllocatorWithDefaultOptions allocator;

	// Retrive input tensor shape information
	Ort::TypeInfo inputTypeInfo = m_Session.GetInputTypeInfo(0);
	std::vector<long> inputTensorShapeVector = inputTypeInfo.GetTensorTypeAndShapeInfo().GetShape();
	m_IsDynamicInputShape = (inputTensorShapeVector.size() >= 4) && (inputTensorShapeVector[2] == -1 && inputTensorShapeVector[3] == -1); // check for dynamic dimentions

	// allocate and store input node names
	auto inputName = m_Session.GetInputNameAllocated(0, allocator);
	inputNodeNameAllocatedStrings.push_back(std::move(inputName));
	inputNames.push_back(inputNodeNameAllocatedStrings.back().get());

	// allocate and store output node names
	auto outputName = m_Session.GetOutputNameAllocated(0, allocator);
	outputNodeNameAllocatedStrings.push_back(std::move(outputName));
	outputNames.push_back(outputNodeNameAllocatedStrings.back().get());

	// set the expected image input shape based on the model's input tensor
	if (inputTensorShapeVector.size() >= 4) {
		m_InputSize = cv::Size(static_cast<int>(inputTensorShapeVector[3]), static_cast<int>(inputTensorShapeVector[2]));
	}
	else {
		throw std::runtime_error("ONNXYolov11: Invalid input tensor shape.");
	}

	// get the number of input and output nodes
	numInputNodes = m_Session.GetInputCount();
	numOutputNodes = m_Session.GetOutputCount();

	// load class names and generate corresponding colours
	classNames = ObjectDetectionHelpers::GetClassNames(labelsPath);
	classColours = ObjectDetectionHelpers::GenerateColours(classNames);

	m_Logger->EnterLog(LogLevel::Info, "ONNXYolov11: Model loaded successfully.");
}

ONNXYolov11::~ONNXYolov11()
{
}

std::vector<std::string> ONNXYolov11::GetAvailableREPs()
{
	std::vector<std::string> availableProviders = Ort::GetAvailableProviders();
	for (std::string rep : availableProviders) {
		m_Logger->EnterLog("Available ONNX Runtime Execution Provider: " + rep);
	}
	return availableProviders;
}

std::vector<Detection> ONNXYolov11::Detect(std::shared_ptr<Frame> frame)
{
	return std::vector<Detection>();
}

cv::Size ONNXYolov11::GetExpectedInputSize()
{
	return m_InputSize;
}

void ONNXYolov11::setONNXREP(std::string onnxREP)
{
	m_ONNXREP = onnxREP;
}

void ONNXYolov11::setConfThreshold(float confThreshold)
{
	m_ConfThreshold = confThreshold;
}

void ONNXYolov11::setIouThreshold(float iouThreshold)
{
	m_IouThreshold = iouThreshold;
}

float ONNXYolov11::getConfThreshold()
{
	return m_ConfThreshold;
}

float ONNXYolov11::getIouThreshold()
{
	return m_IouThreshold;
}

std::shared_ptr<Frame> ONNXYolov11::preprocess(std::shared_ptr<Frame> frame, float* blob, std::vector<long> inputTensorShape)
{
	std::shared_ptr<Frame> resizedFrame = std::make_shared<Frame>();
	
	// Get cv::Mat from Frame
	cv::Mat& inputMat = *frame.get(); // Assuming Frame has a public cv::Mat member named 'mat'
	cv::Mat& outputMat = *resizedFrame.get(); // Assuming Frame has a public cv::Mat member named 'mat'

	// Use cv::Mat references for LetterBox
	ObjectDetectionHelpers::LetterBox(inputMat, outputMat, m_InputSize, cv::Scalar(114, 114, 114), m_IsDynamicInputShape, false, false, true);

	// convert BGR to RGB (YOLOv11 expects RGB input)
	// cv::cvtColor(outputMat, outputMat, cv::COLOR_BGR2RGB);

	return resizedFrame;
}

std::vector<Detection> ONNXYolov11::postprocess(cv::Size originalImageSize, cv::Size resizedImageShape, std::vector<Ort::Value> outputTensors, float confThreshold, float iouThreshold)
{
	return std::vector<Detection>();
}
