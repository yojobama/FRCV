#pragma once
#include "ISink.h"
#include "ISource.h"
#include "IDetectionBackend.h"
#include <memory>

class ObjectDetectionSink : public ISink, public ISource
{
public:
	// takes ownership of an already-Load()-ed backend; ObjectDetectionSink itself has no
	// knowledge of which concrete backend it holds - RKNN vs ONNX Runtime is entirely the
	// caller's (Manager's) decision, matching how ApriltagDetector doesn't know about its own
	// CPU-vs-Vulkan backend distinction either
	ObjectDetectionSink(std::shared_ptr<Logger> logger, std::string id, std::shared_ptr<IDetectionBackend> backend);

	// ROADMAP.md Phase 7 (driver mode) - see ApriltagDetector's own identical accessor for the
	// full rationale; same semantics here (skip inference, keep streaming raw video).
	void SetDriverMode(bool enabled) { m_DriverMode = enabled; }
	bool GetDriverMode() const { return m_DriverMode; }

private:
	void Process(const std::vector<SourceResult>& results) override;

	std::shared_ptr<IDetectionBackend> m_Backend;
	std::shared_ptr<Logger> m_Logger;
	bool m_DriverMode = false;
};
