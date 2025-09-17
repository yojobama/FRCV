#include "IObjectDetectionSink.h"

IObjectDetectionSink::IObjectDetectionSink(Logger* logger, PreProcessor* preProcessor, FramePool* framePool) : ISink(logger)
{
}

IObjectDetectionSink::~IObjectDetectionSink()
{
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
	return "Object Detection Sink operational.";
}