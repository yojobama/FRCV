#pragma once
#include "ISink.h"
class PreProcessor;

// TODO: add a map of class id and object name
class IObjectDetectionSink : public ISink
{
public:
	IObjectDetectionSink(Logger* logger, PreProcessor* preProcessor, FramePool* framePool);
	~IObjectDetectionSink();
	std::string GetStatus() override;
	virtual void CreatePreview();
private:
	virtual void ProcessFrame() override = 0;
};
