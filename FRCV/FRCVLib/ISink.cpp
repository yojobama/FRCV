#include "ISink.h"
#include "Frame.h"

ISink::ISink(Logger* logger) : logger(logger), source(nullptr) {
    if (logger) logger->enterLog("ISink constructed");
}

std::string ISink::getStatus() {
    return nullptr;
}

void* ISink::sinkThreadStart(void* pReference)
{
    ((ISink*)pReference)->sinkThreadProc();
    return nullptr;
}

void ISink::changeThreadStatus(bool threadWantedAlive)
{
    if (threadWantedAlive) {
        shouldTerminate = false;
        pthread_create(&thread, NULL, sinkThreadStart, this);
    }
    else {
        if (thread) {
            shouldTerminate = true;
            while (shouldTerminate) sleep(1);
        }
    }
}

void ISink::sinkThreadProc()
{
    while (!shouldTerminate) {
        // do stuff
        processFrame();
    }
    shouldTerminate = false;
}

string ISink::getCurrentResults()
{
    return results;
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