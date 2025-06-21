#pragma once

#include "Frame.h"
#include "Logger.h"
#include <vector>
#include "ISource.h"
#include <string>

class ISink {
public:
    ISink(Logger* logger);
    virtual ~ISink() = default;
    virtual std::string getResults() = 0; // need to return the resu;ts in a json representation
	bool bindSource(ISource* source);
	bool unbindSource();
protected:
	ISource* source;
    Logger* logger;
};

