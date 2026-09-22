#pragma once

#include "Logger.h"
#include <vector>
#include "ISource.h"
#include <string>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <thread>
#include "SourceResult.h"

class ISink
{
public:
    ISink(std::shared_ptr<Logger> p_Logger, int maxSources, bool requireJson, bool requireFrame, std::string id);
    virtual ~ISink();

    std::string GetID();

    bool BindSource(std::shared_ptr<ISource> p_Source);
	bool UnbindSource(std::string sourceID);
    void Toggle(bool threadWantedAlive);
	bool GetToggleStatus();

protected:

    virtual void Process(const std::vector<SourceResult>& sources) = 0;
private:
    std::shared_ptr<Logger> m_Logger;

    void ProcessingThreadLoop();
    // called (via a listener registered on each bound source) whenever any bound source
    // publishes a new result; wakes ProcessingThreadLoop instead of it polling frame counts
    void NotifyDataAvailable();

    std::jthread m_Thread;
    std::atomic<bool> m_ShouldTerminate{ false };

    std::mutex m_WakeMutex;
    std::condition_variable m_WakeCV;
    bool m_DataAvailable = false;

    // shared with the lambdas registered via ISource::AddResultListener; flipped to false in
    // ~ISink so a listener firing after this sink is destroyed does not touch a dangling `this`
    std::shared_ptr<std::atomic<bool>> m_AliveFlag = std::make_shared<std::atomic<bool>>(true);

    std::string m_ID;

    bool m_ToggleState = false;

    bool m_RequireJson;
    bool m_RequireFrame;
    int m_MaxSources;

    // uint64_t, matching ISource::GetCurrentFrameCount()'s return type exactly - this used to be
    // a plain int compared against a uint64_t, which silently truncated (rather than failing to
    // compile) once a source passed roughly 2^31 frames, and could then compare unequal to the
    // real count forever afterward.
    std::vector<std::pair<std::shared_ptr<ISource>, uint64_t>> m_Sources;
};

