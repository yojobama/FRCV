#pragma once

#include "ISink.h"
#include "IFrameSource.h"
#include "Logger.h"
#include "Frame.h"
#include <vector>

template<class Result>
class IPipeline
{
public:
	IPipeline(Logger* logger);
	~IPipeline();
private:
	std::vector<Result> getResults();
};

