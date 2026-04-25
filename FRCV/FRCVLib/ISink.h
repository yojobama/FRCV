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


class ISink 
{
public:
    ISink(Logger* p_Logger, int maxSources, bool requireJson, bool requireFrame, std::string id);
    ~ISink() = default;

    std::string GetID();

    bool BindSource(std::shared_ptr<ISource> p_Source);
	bool UnbindSource(std::string sourceID);
    virtual std::string GetStatus();
    void Toggle(bool threadWantedAlive);
	bool GetToggleStatus();

protected:
    
    Logger* m_Logger;
    virtual void Process(std::vector<SourceResult> sources) = 0;
    std::shared_ptr<Frame> m_PreviewFrame;
private:
    static void* InvokeProcessingThread(void* p_Reference);
    void ProcessingThreadLoop();
    
    pthread_t m_Thread;
    bool m_ShouldTerminate;
    uint64_t m_LastFrameCount;
    
    std::string m_ID;

    bool m_ToggleState = false;

    bool m_RequireJson;
    bool m_RequireFrame;
    int m_MaxSources;

    std::vector<std::pair<std::shared_ptr<ISource>, int>> m_Sources;
};

