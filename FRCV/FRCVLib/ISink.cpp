#include "ISink.h"
#include "Frame.h"

ISink::ISink(std::shared_ptr<Logger> p_Logger, int maxSources, bool requireJson, bool requireFrame, std::string id) : m_Logger(p_Logger) {
    if (m_Logger) m_Logger->EnterLog("ISink constructed");
    m_MaxSources = maxSources;
    m_RequireJson = requireJson;
    m_RequireFrame = requireFrame;
    m_ID = id;
}

std::string ISink::GetStatus() {
    return "";
}

void* ISink::InvokeProcessingThread(void* p_Reference)
{
    ((ISink*)p_Reference)->ProcessingThreadLoop();
    return nullptr;
}

void ISink::Toggle(bool toggle)
{
    if (toggle) {
        m_ShouldTerminate = false;
        pthread_create(&m_Thread, NULL, InvokeProcessingThread, this);
		m_ToggleState = true;
    }
    else {
        if (m_Thread) {
            m_ShouldTerminate = true;
            pthread_join(m_Thread, NULL);
			m_ToggleState = false;
        }
    }
}

bool ISink::GetToggleStatus()
{
    return m_ToggleState;
}

void ISink::ProcessingThreadLoop()
{
    while (!m_ShouldTerminate) {
        bool wasUpdated = false;
        while (!wasUpdated) {
            for (auto& sourcePair : m_Sources) {
                auto& source = sourcePair.first;
                int& lastFrameCount = sourcePair.second;

                if (source->GetCurrentFrameCount() != lastFrameCount) {
                    lastFrameCount = source->GetCurrentFrameCount();
                    wasUpdated = true;
                }
            }
        }

        std::vector<SourceResult> sources;
        for (auto& sourcePair : m_Sources) {
                        auto& source = sourcePair.first;
            int& lastFrameCount = sourcePair.second;

            if (source->GetCurrentFrameCount() != lastFrameCount) {
                lastFrameCount = source->GetCurrentFrameCount();
                sources.push_back(source->GetLatestResult(m_RequireFrame, m_RequireJson));
            }
        }

        Process(sources);
    }
    m_ShouldTerminate = false;
	pthread_exit(NULL);
}

std::string ISink::GetID()
{
    return m_ID;
}

bool ISink::BindSource(std::shared_ptr<ISource> p_Source) {
    if (m_Logger) m_Logger->EnterLog("ISink::BindSource called");

    if (p_Source && m_Sources.size() < m_MaxSources) {
        m_Sources.push_back(std::make_pair(p_Source, 0));
        return true;
    }
    
    m_Logger->EnterLog(LogLevel::Error, "ISink::BindSource: Source is null");

    return false;
}

bool ISink::UnbindSource(std::string sourceID) {
    if (m_Logger) m_Logger->EnterLog("ISink::UnbindSource called");

    for (int i = 0; i < m_Sources.size(); i++) {
        if (m_Sources[i].first->GetID() == sourceID) {
            m_Sources.erase(m_Sources.begin() + i);
            return true;
        }
    }

    return false;
}