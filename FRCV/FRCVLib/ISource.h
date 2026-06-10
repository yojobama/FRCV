#pragma once

#include <opencv2/opencv.hpp>
#include "Logger.h"
#include "SourceResult.h"
#include <mutex>
#include <pthread.h>
#include <queue>

class Frame;

class ISource
{
public:
	ISource(Logger* p_Logger, std::string m_ID);
	virtual ~ISource();
	SourceResult GetLatestResult(bool requireFrame, bool requireJson);
	std::string GetID();
	
	void Toggle(bool threadWantedAlive);
	uint64_t GetCurrentFrameCount();
	bool GetToggleStatus();
protected:
	void SetLatestResult(SourceResult result);
	uint64_t m_FrameCount = 0;
	virtual void CaptureFrame() = 0;
	Logger* m_Logger;
	bool m_DoNotLoadCaptureThread = false;
private:
	SourceResult m_LatestResult;

	static void* SourceThreadStart(void* p_Reference);
	void SourceThreadProc();
	std::mutex m_ResultLock;
	pthread_t m_Thread;
	bool m_ShouldTerminate;
	bool m_ToggleState = false;

	std::string m_ID;
};

