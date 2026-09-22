#include "ISource.h"
#include "SourceResult.h"
#include "CpuAffinity.h"

ISource::ISource(std::shared_ptr<Logger> p_Logger, std::string m_ID)
	: m_ResultLock() // Initialize m_ResultLock
{
	this->m_Logger = p_Logger;
	this->m_ID = m_ID;
}

ISource::~ISource()
{
	// previously a no-op: the capture thread outlived this object with nothing to stop it.
	if (m_Thread.joinable()) {
		m_ShouldTerminate = true;
		m_Thread.join();
	}
}

std::string ISource::GetID()
{
	return m_ID;
}

void ISource::Toggle(bool threadWantedAlive)
{
	if (m_DoNotLoadCaptureThread) return;
	// idempotent: calling Toggle(true) while already running previously spawned a SECOND
	// capture thread without stopping the first - the new thread handle overwrote m_Thread, so
	// the original was never join-able again (leaked, not hung: m_ShouldTerminate is shared,
	// so it would still observe a later stop request and exit - just never get reaped).
	if (threadWantedAlive == m_ToggleState) return;

	if (threadWantedAlive) {
		m_ShouldTerminate = false;
		m_Thread = std::jthread([this] { SourceThreadProc(); });
		m_ToggleState = true;
	} else {
		if (m_Thread.joinable()) {
			m_ShouldTerminate = true;
			m_Thread.join(); // Wait for the thread to terminate
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

void ISource::AddResultListener(std::function<void()> listener)
{
	std::lock_guard<std::mutex> guard(m_ListenersMutex);
	m_Listeners.push_back(std::move(listener));
}

void ISource::RegisterFrameConsumer(const std::string& sinkId, bool requiresFrame, std::function<bool()> isActive)
{
	std::lock_guard<std::mutex> guard(m_FrameConsumersMutex);
	m_FrameConsumers[sinkId] = FrameConsumer{ requiresFrame, std::move(isActive) };
}

void ISource::UnregisterFrameConsumer(const std::string& sinkId)
{
	std::lock_guard<std::mutex> guard(m_FrameConsumersMutex);
	m_FrameConsumers.erase(sinkId);
}

bool ISource::HasActiveFrameConsumer() const
{
	std::lock_guard<std::mutex> guard(m_FrameConsumersMutex);
	for (const auto& [id, consumer] : m_FrameConsumers) {
		if (consumer.requiresFrame && consumer.isActive()) return true;
	}
	return false;
}

void ISource::SetLatestResult(SourceResult result)
{
	{
		std::lock_guard<std::mutex> guard(m_ResultLock); // Use RAII for mutex locking
		result.producedTimeUs = SourceResult::NowUs();
		// a producer that didn't pass an explicit capture timestamp (the two-arg SourceResult
		// constructor) gets producedTimeUs standing in for captureTimeUs too - see SourceResult.h
		if (result.captureTimeUs == 0) result.captureTimeUs = result.producedTimeUs;
		result.frameNumber = m_FrameCount + 1; // matches the m_FrameCount++ below
		m_LatestResult = result;
		// m_FrameCount is what ISink::ProcessingThreadLoop actually checks to decide whether a
		// source has anything new (source->GetCurrentFrameCount() != lastFrameCount) - it needs
		// to change on every published result, so it belongs here, not in each subclass's own
		// CaptureFrame(). Confirmed by actually running the pipeline: VideoFileSource remembered
		// to bump it itself, but CameraFrameSource and ImageFileFrameSource did not, so a
		// sink bound to either of those NEVER saw a result to process - the wake-up notification
		// below fired correctly (that part came from the busy-wait fix earlier), but
		// ProcessingThreadLoop's own frame-count check silently filtered every source out before
		// Process() could ever be called.
		m_FrameCount++;
	}

	// notify bound sinks outside the result lock so a listener can safely call back
	// into this source (e.g. GetLatestResult) without deadlocking
	std::vector<std::function<void()>> listenersCopy;
	{
		std::lock_guard<std::mutex> guard(m_ListenersMutex);
		listenersCopy = m_Listeners;
	}
	for (auto& listener : listenersCopy) {
		listener();
	}
}

SourceResult ISource::GetLatestResult(bool requireFrame, bool requireJson)
{
	std::lock_guard<std::mutex> guard(m_ResultLock); // Use RAII for mutex locking
	// "at least", not "exactly": a sink declares what it needs, not what the source may also
	// produce. This used to require an exact match, which silently broke any sink bound to a
	// dual-producing source (e.g. ApriltagDetector, which always sets both frame and json) if
	// the sink only asked for one of the two - WebRTCSink (frame-only) bound to a detector's
	// annotated output being exactly that case, and exactly the "different stages" composition
	// the WebRTC feature depends on.
	if ((!requireJson || m_LatestResult.json.has_value()) && (!requireFrame || m_LatestResult.frame.has_value()))
		return m_LatestResult;
	return SourceResult();
}

SourceResult ISource::GetLatestResult()
{
	std::lock_guard<std::mutex> guard(m_ResultLock);
	return m_LatestResult;
}

void ISource::SourceThreadProc()
{
	// capture + colour conversion is CPU-heavy and latency-sensitive - keep it off the slow
	// efficiency cores when this is a big.LITTLE SoC (see CpuAffinity's own comment on why this
	// isn't hardcoded to a specific core index)
	CpuAffinity::PinCurrentThreadToPerformanceCores();
	OnCaptureThreadStart();
	while (!m_ShouldTerminate) {
		CaptureFrame();
	}
	OnCaptureThreadStop();
	m_ShouldTerminate = false;
}

void ISource::CaptureFrame() {
	// this is a default implementation
}
