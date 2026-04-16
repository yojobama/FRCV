#pragma once
#include "Logger.h"
#include "FramePool.h"
#include "FrameSpec.h"
#include <mutex>
#include <pthread.h>
#include <queue>

class Frame;
class SourceResult;

class ISource
{
public:
	ISource(FramePool* p_FramePool, Logger* p_Logger, std::string m_ID);
	virtual ~ISource();
	//virtual std::shared_ptr<Frame> GetLatestFrame();
	//virtual std::shared_ptr<Frame> GetLatestFrame(bool forceNewFrame);
	SourceResult GetLatestResult(bool requireFrame, bool requireJson);
	void SetLatestResult(SourceResult result);
	
	std::string GetID();
	
	void Toggle(bool threadWantedAlive);
	uint64_t GetCurrentFrameCount();
	bool GetToggleStatus();
protected:
	uint64_t m_FrameCount = 0;
	virtual void CaptureFrame() = 0;
	FramePool* m_FramePool;
	Logger* m_Logger;
	FrameSpec m_FrameSpec;
	bool m_DoNotLoadCaptureThread = false;
private:
	SourceResult m_LatestResult;

	static void* SourceThreadStart(void* p_Reference);
	void SourceThreadProc();
	std::mutex m_Lock;
	pthread_t m_Thread;
	bool m_ShouldTerminate;
	bool m_ToggleState = false;

	std::string m_ID;
};

