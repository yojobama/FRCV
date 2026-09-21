#include "ISink.h"

ISink::ISink(std::shared_ptr<Logger> p_Logger, int maxSources, bool requireJson, bool requireFrame, std::string id) : m_Logger(p_Logger) {
    if (m_Logger) m_Logger->EnterLog("ISink constructed");
    m_MaxSources = maxSources;
    m_RequireJson = requireJson;
    m_RequireFrame = requireFrame;
    m_ID = id;
}

ISink::~ISink()
{
    *m_AliveFlag = false;
}

void* ISink::InvokeProcessingThread(void* p_Reference)
{
    ((ISink*)p_Reference)->ProcessingThreadLoop();
    return nullptr;
}

void ISink::Toggle(bool toggle)
{
    // idempotent, for the same reason as ISource::Toggle: calling Toggle(true) while already
    // running previously spawned a second processing thread without stopping the first
    if (toggle == m_ToggleState) return;

    if (toggle) {
        m_ShouldTerminate = false;
        pthread_create(&m_Thread, NULL, InvokeProcessingThread, this);
		m_ToggleState = true;
    }
    else {
        if (m_Thread) {
            m_ShouldTerminate = true;
            {
                // wake the loop immediately so it observes m_ShouldTerminate instead of
                // waiting out its full timeout before it can be joined
                std::lock_guard<std::mutex> guard(m_WakeMutex);
                m_DataAvailable = true;
            }
            m_WakeCV.notify_one();
            pthread_join(m_Thread, NULL);
			m_ToggleState = false;
        }
    }
}

bool ISink::GetToggleStatus()
{
    return m_ToggleState;
}

void ISink::NotifyDataAvailable()
{
    std::lock_guard<std::mutex> guard(m_WakeMutex);
    m_DataAvailable = true;
    m_WakeCV.notify_one();
}

void ISink::ProcessingThreadLoop()
{
    // safety-net poll interval: if a bound source stalls or a notification is missed,
    // the loop still re-checks frame counts periodically instead of hanging forever
    const auto pollTimeout = std::chrono::milliseconds(100);

    while (!m_ShouldTerminate) {
        {
            std::unique_lock<std::mutex> lock(m_WakeMutex);
            m_WakeCV.wait_for(lock, pollTimeout, [this] { return m_DataAvailable || m_ShouldTerminate.load(); });
            m_DataAvailable = false;
        }

        if (m_ShouldTerminate) break;

        std::vector<SourceResult> sources;
        for (auto& sourcePair : m_Sources) {
            auto& source = sourcePair.first;
            int& lastFrameCount = sourcePair.second;

            if (source->GetCurrentFrameCount() != lastFrameCount) {
                lastFrameCount = source->GetCurrentFrameCount();
                SourceResult sourceResult = source->GetLatestResult(m_RequireFrame, m_RequireJson);
                sourceResult.sourceId = source->GetID();
                sources.push_back(sourceResult);
            }
        }

        if (!sources.empty()) {
            Process(sources);
        }
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
        std::weak_ptr<std::atomic<bool>> aliveFlag = m_AliveFlag;
        p_Source->AddResultListener([this, aliveFlag] {
            if (auto alive = aliveFlag.lock(); alive && *alive) {
                NotifyDataAvailable();
            }
        });
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
