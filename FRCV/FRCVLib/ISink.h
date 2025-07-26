#pragma once

#include "Logger.h"
#include <vector>
#include "ISource.h"
#include <string>
#include <pthread.h>
#include <mutex>

class Frame;

using namespace std;

class ISink {
public:
    ISink(Logger* p_Logger);
    ~ISink() = default;
    string GetCurrentResults();
    bool BindSource(ISource* p_Source);
	bool UnbindSource();
    virtual string GetStatus();
    void ChangeThreadStatus(bool threadWantedAlive);
protected:
    string m_Results;
	ISource* m_Source;
    Logger* m_Logger;
    string m_CurrentResults;
    virtual void ProcessFrame() = 0;
private:
    static void* SinkThreadStart(void* p_Reference);
    void SinkThreadProc();
    mutex m_Lock;
    pthread_t m_Thread;
    bool m_ShouldTerminate;
};

