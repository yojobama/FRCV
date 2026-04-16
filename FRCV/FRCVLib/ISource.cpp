#include "ISource.h"
#include "Frame.h"
#include "SourceResult.h"

ISource::ISource(FramePool* p_FramePool, Logger* p_Logger, std::string m_ID)
	: m_FrameSpec(0, 0, 0), m_Lock() // Initialize m_FrameSpec with default values
{
	this->m_FramePool = p_FramePool;
	this->m_Logger = p_Logger;
	this->m_ID = m_ID;
}

ISource::~ISource()
{
}

//std::shared_ptr<Frame> ISource::GetLatestFrame(bool forceNewFrame)
//{
//	std::lock_guard<std::mutex> guard(m_Lock); // Use RAII for mutex locking
//
//	if (forceNewFrame) {
//		m_Logger->EnterLog(LogLevel::Info, "Forcing new frame capture");
//		CaptureFrame();
//	}
//	while (!m_Frames.empty()) {
//		std::shared_ptr<Frame> p_FrontFrame = m_Frames.front();
//		if (p_FrontFrame.use_count() > 1) {
//			// Frame is still in use, return the latest frame
//			return m_Frames.back();
//		} else {
//			// Remove unused frame from the queue
//			m_Frames.pop();
//		}
//	}
//
//	// If no valid frames are available, log an error and return nullptr
//	m_Logger->EnterLog(LogLevel::Error, "Frame queue is empty");
//	return nullptr;
//}
//
//std::shared_ptr<Frame> ISource::GetLatestFrame()
//{
//	std::lock_guard<std::mutex> guard(m_Lock); // Use RAII for mutex locking
//
//	while (!m_Frames.empty()) {
//		std::shared_ptr<Frame> p_FrontFrame = m_Frames.front();
//		if (p_FrontFrame.use_count() > 1) {
//			// Frame is still in use, return the latest frame
//			return m_Frames.back();
//		}
//		else {
//			// Remove unused frame from the queue
//			m_Frames.pop();
//		}
//	}
//
//	// If no valid frames are available, log an error and return nullptr
//	m_Logger->EnterLog(LogLevel::Error, "Frame queue is empty");
//	return nullptr;
//}

void ISource::Toggle(bool threadWantedAlive)
{
	if (threadWantedAlive && !m_DoNotLoadCaptureThread) {
		m_ShouldTerminate = false;
		pthread_create(&m_Thread, NULL, SourceThreadStart, this);
		m_ToggleState = true;
	} else if (!m_DoNotLoadCaptureThread) {
		if (m_Thread) {
			m_ShouldTerminate = true;
			pthread_join(m_Thread, NULL); // Wait for the thread to terminate
			m_ToggleState = false;
		}
	}
}

uint64_t ISource::GetCurrentFrameCount()
{
	return m_FrameCount;
}

bool ISource::GetToggleStatus()
{
	return m_ToggleState;
}

SourceResult ISource::GetLatestResult(bool requireFrame, bool requireJson)
{
	std::lock_guard<std::mutex> guard(m_Lock); // Use RAII for mutex locking
	if ((m_LatestResult.json.has_value() == requireJson) && (m_LatestResult.frame.has_value() == requireFrame))
		return m_LatestResult;
	return SourceResult();
}

void* ISource::SourceThreadStart(void* p_Reference)
{
	((ISource*)p_Reference)->SourceThreadProc();
	return NULL;
}

void ISource::SourceThreadProc()
{
	while (!m_ShouldTerminate) {
		CaptureFrame();
	}
	m_ShouldTerminate = false;
}
