#pragma once

#include <opencv2/opencv.hpp>
#include "Logger.h"
#include "SourceResult.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
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
	// Written under m_ResultLock (in SetLatestResult) but read WITHOUT it by
	// GetCurrentFrameCount(), called cross-thread from ISink::ProcessingThreadLoop - a plain
	// uint64_t here was a genuine data race (undefined behaviour, not just "usually fine"),
	// confirmed as a real bug worth fixing rather than a false-positive tidiness concern.
	std::atomic<uint64_t> m_FrameCount{ 0 };
	virtual void CaptureFrame(); // TODO: think about: should this be removed?
	// Invoked once each, at the very start/end of the capture thread's lifetime (not per-frame).
	// A default no-op here so every existing subclass keeps compiling unchanged; real camera
	// backends override these for state that must live on the capture thread itself - MF's
	// CoInitializeEx/MFStartup (COM apartment + thread-local state), V4L2's STREAMON/STREAMOFF.
	// Bolting this on after backends exist would need a thread-local workaround instead.
	virtual void OnCaptureThreadStart() {}
	virtual void OnCaptureThreadStop() {}
	std::shared_ptr<Logger> m_Logger;
	bool m_DoNotLoadCaptureThread = false;
private:
	SourceResult m_LatestResult;

	void SourceThreadProc();
	std::mutex m_ResultLock;
	std::jthread m_Thread;
	std::atomic<bool> m_ShouldTerminate{ false };
	bool m_ToggleState = false;

	std::mutex m_ListenersMutex;
	std::vector<std::function<void()>> m_Listeners;

	std::string m_ID;
};

