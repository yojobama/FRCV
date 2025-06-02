#pragma once
#include "IPipeline.h"
#include "IFrameSource.h"
#include "ISink.h"

template<class Result>
class SingleSourcePipeline : IPipeline
{
public:
	SingleSourcePipeline(IFrameSource* frameSource, ISink* sink, Logger* logger) : IPipeline(logger)
	~SingleSourcePipeline();
	std::vector<Result>* getResults();
private:
	std::vector<Result>* 
	IFrameSource* frameSource;
	ISink* sink;
	Logger* logger;
	Frame* currentFrame;
};

