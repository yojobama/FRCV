#pragma once
#include "IPipeline.h"
#include "IFrameSource.h"
#include "ISink.h"
#include <vector>

template<class Result>
class SingleSourcePipeline : public IPipeline<Result> // Fixed inheritance
{
public:
	SingleSourcePipeline(IFrameSource* frameSource, ISink<Result>* sink, Logger* logger) 
		: IPipeline<Result>(logger), frameSource(frameSource), sink(sink), logger(logger), currentFrame(nullptr) {}
	~SingleSourcePipeline();
	std::vector<Result>* getResults();
private:
	std::vector<Result>* results; // Fixed missing member declaration
	IFrameSource* frameSource;
	ISink<Result>* sink; // Fixed missing template argument
	Logger* logger;
	Frame* currentFrame;
};

