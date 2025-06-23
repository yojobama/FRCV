#pragma once

#include "Logger.h"
#include <vector>
#include "ISource.h"
#include <string>

class Frame;

using namespace std;

class ISink {
public:
    ISink(Logger* logger);
    ~ISink() = default;
    string getCurrentResults();
    bool bindSource(ISource* source);
	bool unbindSource();
    virtual string getStatus();
protected:
	ISource* source;
    Logger* logger;
    virtual void captureFrame() = 0;
};

