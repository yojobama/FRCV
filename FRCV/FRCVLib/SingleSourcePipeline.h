#pragma once
#include "IPipeline.h"
#include "IFrameSource.h"
#include "ISink.h"
#include <vector>

template<class Result>
class SingleSourcePipeline : public IPipeline<Result> // Fixed inheritance
{
public:
	SingleSourcePipeline(ISource* frameSource, ISink<Result>* sink, Logger* logger) 
		: IPipeline<Result>(logger), frameSource(frameSource), sink(sink), logger(logger), currentFrame(nullptr) {}
	~SingleSourcePipeline();
	std::vector<Result>* getResults();
private:
	std::vector<Result>* results; // Fixed missing member declaration
	ISource* frameSource;
	ISink<Result>* sink; // Fixed missing template argument
	Logger* logger;
	Frame* currentFrame;
};

template<class Result>
inline SingleSourcePipeline<Result>::~SingleSourcePipeline()
{
	logger->enterLog(INFO, "deleting a single source pipeline");
}


template<class Result>
inline std::vector<Result>* SingleSourcePipeline<Result>::getResults()
{
	logger->enterLog(INFO, "single source pipeline retriving frame");
	Frame* frame = frameSource->getFrame();
	logger->enterLog(INFO, "single source pipeline processing frame for results");
	auto results = sink->getResults();
	logger->enterLog(INFO, "single source pipeline returning post processing results");
	return results;
}
