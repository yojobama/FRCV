#pragma once

#include "Frame.h"
#include "Logger.h"
#include <vector>
#include "ISource.h"
#include <string>

using namespace std;

class ISink {
public:
    ISink(Logger* logger);
    ~ISink() = default;
    virtual std::string getResults();
    bool bindSource(ISource* source);
	bool unbindSource();
    virtual string getStatus();
protected:
	ISource* source;
    Logger* logger;
};

