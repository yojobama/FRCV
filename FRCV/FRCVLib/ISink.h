#pragma once

#include "Logger.h"
#include <vector>
#include "ISource.h"
#include <string>
#include <pthread.h>
#include <mutex>
#include <memory>
#include "SourceResult.h"

class Frame;
class StereoSink;

using namespace std;

class ISink 
{
public:
    ISink(Logger* p_Logger, int maxSources, bool requireJson, bool requireFrame);
    ~ISink() = default;

    //string GetCurrentResults();
    
    bool BindSource(std::shared_ptr<ISource> p_Source);
	bool UnbindSource(std::string sourceID);
    virtual string GetStatus();
    void Toggle(bool threadWantedAlive);
	bool GetToggleStatus();

    //void EnablePreview();
    //void DissablePreview();
    //bool GetPreviewStatus();
    //std::shared_ptr<Frame> GetPreviewFrame();
protected:
    //string m_Results;
    
    Logger* m_Logger;
    
    //string m_CurrentResults;
    
    virtual void Process(std::vector<SourceResult> sources) = 0;
    
    //virtual void CreatePreview() = 0;
    
    std::shared_ptr<Frame> m_PreviewFrame;
private:
    static void* InvokeProcessingThread(void* p_Reference);
    void ProcessingThreadLoop();
    
    //mutex m_Lock;
    
    pthread_t m_Thread;
    bool m_ShouldTerminate;
    uint64_t m_LastFrameCount;
    
    //bool m_PreviewEnabled;
	//StereoSink* m_StereoSink;
   
    bool m_ToggleState = false;

    bool m_RequireJson;
    bool m_RequireFrame;
    int m_MaxSources;

    std::vector<std::pair<std::shared_ptr<ISource>, int>> m_Sources;
};

