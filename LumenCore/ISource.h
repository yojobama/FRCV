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
#include <unordered_map>
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

	// Called by ISink::BindSource/UnbindSource so a dual-role node (ApriltagDetector/
	// ObjectDetectionSink) can tell whether anything downstream actually wants its FRAME (as
	// opposed to just its JSON, e.g. NetworkTablesSink) right now - see HasActiveFrameConsumer's
	// own comment for what this enables. isActive is called lazily (not cached) so it always
	// reflects the consumer's CURRENT toggled-on state, not a snapshot from bind time; it must
	// return false once the registering sink is destroyed (guard it with the same alive-flag
	// idiom AddResultListener's own callers already use), not dangle a raw `this`.
	void RegisterFrameConsumer(const std::string& sinkId, bool requiresFrame, bool requiresColor, std::function<bool()> isActive);
	// removed on an explicit UnbindSource - not on destruction (destruction is instead covered
	// by isActive naturally returning false forever after, same tradeoff AddResultListener
	// already makes: a handful of dead entries across a session's lifetime, never actively
	// leaking meaningfully, versus needing every ISink subclass to unregister on teardown).
	void UnregisterFrameConsumer(const std::string& sinkId);

	// true if at least one bound, frame-requiring consumer is currently active (alive and
	// toggled on) - e.g. a running WebRTCSink bound to this detector's annotated output.
	// ApriltagDetector/ObjectDetectionSink use this to skip drawing annotations onto a frame
	// nothing is actually going to look at (NT4-only publishing needs the JSON, never the
	// image) - real CPU/latency cost on a robot with no live preview open, the common case
	// during an actual match.
	bool HasActiveFrameConsumer() const;
	// true if at least one bound, active consumer needs its frame in COLOUR specifically (a
	// WebRTC/Mjpeg/Record sink - always true unless a subclass explicitly opts out via ISink's
	// own requireColor constructor argument). ApriltagDetector is the one existing opt-out - it
	// only ever calls Frame::AsGray() on a camera's raw frame, never AsBgr(), so registering it
	// as requiring colour here would defeat V4l2CameraBackend's own decode-straight-to-gray
	// optimization (see CameraSource.cpp's own comment) for the single most common real pipeline
	// shape (camera -> AprilTag detector -> NT4, no preview/recording bound) - confirmed the hard
	// way: before this distinction existed, a bound-but-gray-only detector's own (correct)
	// requiresFrame=true made HasActiveFrameConsumer() true unconditionally, so preferGray never
	// actually engaged for exactly the case it was built for.
	bool HasActiveColorFrameConsumer() const;
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

	struct FrameConsumer {
		bool requiresFrame;
		bool requiresColor;
		std::function<bool()> isActive;
	};
	mutable std::mutex m_FrameConsumersMutex;
	std::unordered_map<std::string, FrameConsumer> m_FrameConsumers;

	std::string m_ID;
};

