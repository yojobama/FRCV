#pragma once

#include <opencv2/opencv.hpp>
#include "Logger.h"
#include "SourceResult.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <vector>

class ISource
{
public:
	ISource(std::shared_ptr<Logger> p_Logger, std::string m_ID);
	virtual ~ISource();
	SourceResult GetLatestResult(bool requireFrame, bool requireJson);
	// returns whatever the source last produced, without gating on which fields are present
	SourceResult GetLatestResult();
	std::string GetID();

	void Toggle(bool threadWantedAlive);
	uint64_t GetCurrentFrameCount();
	bool GetToggleStatus();

	// registers a callback invoked (off the result lock) whenever a new result is published;
	// used by bound ISinks to wake their processing thread instead of polling
	void AddResultListener(std::function<void()> listener);
protected:
	void SetLatestResult(SourceResult result);
	uint64_t m_FrameCount = 0;
	virtual void CaptureFrame(); // TODO: think about: should this be removed?
	std::shared_ptr<Logger> m_Logger;
	bool m_DoNotLoadCaptureThread = false;
private:
	SourceResult m_LatestResult;

	static void* SourceThreadStart(void* p_Reference);
	void SourceThreadProc();
	std::mutex m_ResultLock;
	pthread_t m_Thread = 0;
	std::atomic<bool> m_ShouldTerminate{ false };
	bool m_ToggleState = false;

	std::mutex m_ListenersMutex;
	std::vector<std::function<void()>> m_Listeners;

	std::string m_ID;
};

