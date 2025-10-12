#include "ONNXSink.h"
#include "ONNX_YOLO11.hpp"
#include "Frame.h"

ONNXSink::ONNXSink(std::string onnxREP, ObjectDetectionModelParameters modelParameters, Logger* logger, PreProcessor* preProcessor, FramePool* framePool)
	: IObjectDetectionSink(modelParameters, logger, preProcessor, framePool)
{
	m_ONNXREP = onnxREP;
	m_Detector = new YOLO11Detector(modelParameters.modelPath, modelParameters.labelsPath); // TODO: This is a sample imnplementation, fix it later
}

ONNXSink::~ONNXSink() // SOLVE: There is a problem with the destructor, it makes the compilation process not to work :(
{
	// TODO: Implement the destructor (and check how to make it work alongside the base constructor)
}

// Add missing method implementation to fix vtable error
std::string ONNXSink::GetStatus()
{
	// TODO: Implement status reporting for ONNXSink
	return "ONNXSink status not implemented";
}

void ONNXSink::ProcessFrame()
{
	std::shared_ptr<Frame> src = m_Source->GetLatestFrame();
	std::string returnString = "{\"detections\":[";


	if (src != nullptr)
	{
		std::vector<Detection> detections = m_Detector->detect(*src.get());
		
		for (size_t i = 0; i < detections.size(); ++i) {
			const Detection& det = detections[i];
			
			returnString += "{";
			returnString += "\"bbox\":{";
			returnString += "\"x\":" + std::to_string(det.box.x) + ",";
			returnString += "\"y\":" + std::to_string(det.box.y) + ",";
			returnString += "\"width\":" + std::to_string(det.box.width) + ",";
			returnString += "\"height\":" + std::to_string(det.box.height);
			returnString += "},";
			returnString += "\"confidence\":" + std::to_string(det.conf) + ",";
			returnString += "\"class_id\":" + std::to_string(det.classId);
			returnString += "}";
			
			if (i != detections.size() - 1) {
				returnString += ",";
			}
		}
	}
	else
	{
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "ONNXSink::ProcessFrame: Frame is null");
	}
	
	returnString += "]}";
	m_Results = returnString;
}