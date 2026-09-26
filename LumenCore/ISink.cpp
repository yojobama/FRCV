#include "ISink.h"
#include "CpuAffinity.h"

ISink::ISink(std::shared_ptr<Logger> p_Logger, int maxSources, bool requireJson, bool requireFrame, std::string id, bool requireColor) : m_Logger(p_Logger) {
    if (m_Logger) m_Logger->EnterLog("ISink constructed");
    m_MaxSources = maxSources;
    m_RequireJson = requireJson;
    m_RequireFrame = requireFrame;
    m_RequireColor = requireColor;
    m_ID = id;
}

ISink::~ISink()
{
    *m_AliveFlag = false;
    // previously a no-op beyond the alive flag: the processing thread outlived this object
    // with nothing to stop it (see ISource::~ISource, the same bug).
    if (m_Thread.joinable()) {
        m_ShouldTerminate = true;
        {
            std::lock_guard<std::mutex> guard(m_WakeMutex);
            m_DataAvailable = true;
        }
        m_WakeCV.notify_one();
        m_Thread.join();
    }
}

void ISink::Toggle(bool toggle)
{
    // idempotent, for the same reason as ISource::Toggle: calling Toggle(true) while already
    // running previously spawned a second processing thread without stopping the first
    if (toggle == m_ToggleState) return;

    if (toggle) {
        m_ShouldTerminate = false;
        m_Thread = std::jthread([this] { ProcessingThreadLoop(); });
		m_ToggleState = true;
    }
    else {
        if (m_Thread.joinable()) {
            m_ShouldTerminate = true;
            {
                // wake the loop immediately so it observes m_ShouldTerminate instead of
                // waiting out its full timeout before it can be joined
                std::lock_guard<std::mutex> guard(m_WakeMutex);
                m_DataAvailable = true;
            }
            m_WakeCV.notify_one();
            m_Thread.join();
			m_ToggleState = false;
			OnStopped();
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
    // detection/encoding is CPU-heavy and latency-sensitive - keep it off the slow efficiency
    // cores when this is a big.LITTLE SoC (see CpuAffinity's own comment on why this isn't
    // hardcoded to a specific core index)
    CpuAffinity::PinCurrentThreadToPerformanceCores();

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
            uint64_t& lastFrameCount = sourcePair.second;

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
        // see ISource::HasActiveFrameConsumer's own comment - this is what lets a bound
        // detector know whether it's worth annotating a frame for this sink specifically.
        // GetToggleStatus() is only called once the alive flag confirms `this` still exists.
        p_Source->RegisterFrameConsumer(m_ID, m_RequireFrame, m_RequireColor, [this, aliveFlag] {
            auto alive = aliveFlag.lock();
            return alive && *alive && this->GetToggleStatus();
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
            m_Sources[i].first->UnregisterFrameConsumer(m_ID);
            m_Sources.erase(m_Sources.begin() + i);
            return true;
        }
    }

    return false;
}
