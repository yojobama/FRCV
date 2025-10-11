#pragma once
#include "ISink.h"
#include "ObjectDetectionModelParameters.h"
class PreProcessor;

// TODO: add a map of class id and object name
class IObjectDetectionSink : public ISink
{
public:
	IObjectDetectionSink(ObjectDetectionModelParameters modelParameters, Logger* logger, PreProcessor* preProcessor, FramePool* framePool);
	~IObjectDetectionSink();
	std::string GetStatus() override;
	virtual void CreatePreview();
protected:
	ObjectDetectionModelParameters m_ModelParameters;
	PreProcessor* m_PreProcessor;
	FramePool* m_FramePool;
	virtual void ProcessFrame() override = 0;
};
