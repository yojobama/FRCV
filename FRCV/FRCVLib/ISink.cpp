#include "ISink.h"

ISink::ISink(Logger* logger) : logger(logger), source(nullptr) {
    if (logger) logger->enterLog("ISink constructed");
}

bool ISink::bindSource(ISource* source) {
    if (logger) logger->enterLog("ISink::bindSource called");
    if (source == nullptr) {
        logger->enterLog(ERROR, "ISink::bindSource: Source is null");
        return false;
    }
    this->source = source;
    return true;
}

bool ISink::unbindSource() {
    if (logger) logger->enterLog("ISink::unbindSource called");
    if (source == nullptr) {
        logger->enterLog(ERROR, "ISink::unbindSource: Source is already null");
        return false;
    }
    source = nullptr;
    return true;
}