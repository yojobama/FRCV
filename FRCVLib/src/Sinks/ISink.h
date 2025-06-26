#pragma once

#include "../Logger.h"
#include <vector>
#include "Sources/ISource.h"
#include <string>
#include <pthread.h>
#include <mutex>

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
    void changeThreadStatus(bool threadWantedAlive);
protected:
    string results;
	ISource* source;
    Logger* logger;
    string currentResults;
    virtual void processFrame() = 0;
private:
    static void* sinkThreadStart(void* pReference);
    void sinkThreadProc();
    mutex lock;
    pthread_t thread;
    bool shouldTerminate;
};

