#pragma once
#include "Frame.h"
#include "Logger.h"
#include <vector>

template<class Result>
class ISink
{
public:
	ISink(Logger* logger);
	~ISink();
	virtual std::vector<Result> getResults(Frame* frame);
private:
	Logger* logger;
};

