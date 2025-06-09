#pragma once

#include "Frame.h"
#include "Logger.h"
#include <vector>

template <typename T>
class ISink {
public:
    ISink(Logger* logger) : logger(logger) {}
    virtual ~ISink() = default;
    virtual std::vector<T> getResults();
protected:
    Logger* logger;
};

