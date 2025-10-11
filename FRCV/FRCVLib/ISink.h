#pragma once

#include "Logger.h"
#include <vector>
#include "ISource.h"
#include <string>
#include <pthread.h>
#include <mutex>
#include <memory>

class Frame;
class StereoSink;

using namespace std;

class ISink 
{
public:
    ISink(Logger* p_Logger);
    ~ISink() = default;
    string GetCurrentResults();
    bool BindSource(ISource* p_Source);
	bool UnbindSource();
    virtual string GetStatus();
    void ChangeThreadStatus(bool threadWantedAlive);

	bool GetActivationStatus();

    void EnablePreview();
    void DissablePreview();
    bool GetPreviewStatus();
    std::shared_ptr<Frame> GetPreviewFrame();
protected:
    string m_Results;
	ISource* m_Source;
    Logger* m_Logger;
    string m_CurrentResults;
    virtual void ProcessFrame() = 0;
    virtual void CreatePreview() = 0;
    std::shared_ptr<Frame> m_PreviewFrame;
private:
    static void* SinkThreadStart(void* p_Reference);
    void SinkThreadProc();
    mutex m_Lock;
    pthread_t m_Thread;
    bool m_ShouldTerminate;
    uint64_t m_LastFrameCount;
    bool m_PreviewEnabled;
	StereoSink* m_StereoSink;
    bool m_Activated = false;
};

