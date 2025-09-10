#include "IObjectDetectionSink.h"

IObjectDetectionSink::IObjectDetectionSink(Logger* logger, PreProcessor* preProcessor, FramePool* framePool)
{
}

IObjectDetectionSink::~IObjectDetectionSink()
{
}

// TODO: Implement, take inspiration from opencv
void IObjectDetectionSink::CreatePreview()
{
	if (!GetPreviewStatus() || m_Source == nullptr) return;


}
