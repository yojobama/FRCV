#include "IObjectDetectionSink.h"

IObjectDetectionSink::IObjectDetectionSink(ObjectDetectionModelParameters modelParameters, Logger* logger, PreProcessor* preProcessor, FramePool* framePool) : ISink(logger)
{
	m_ModelParameters = modelParameters;
	m_Logger->EnterLog("IObjectDetectionSink created with model: " + modelParameters.modelPath);
	m_FramePool = framePool;
	m_Logger->EnterLog("FramePool assigned to IObjectDetectionSink.");
	m_PreProcessor = preProcessor;
	m_Logger->EnterLog("PreProcessor assigned to IObjectDetectionSink.");
}

IObjectDetectionSink::~IObjectDetectionSink()
{
	// TODO: Implement the destructor
}

// TODO: Implement, take inspiration from opencv
void IObjectDetectionSink::CreatePreview()
{
	if (!GetPreviewStatus() || m_Source == nullptr) return;
	// Example implementation (to be replaced with actual logic)
	m_PreviewFrame = m_Source->GetLatestFrame();
}

void IObjectDetectionSink::ProcessFrame()
{
	// Pure virtual, must be implemented by derived classes
}

std::string IObjectDetectionSink::GetStatus()
{
	// TODO: Implement detailed status reporting
	return "Object Detection Sink operational.";
}