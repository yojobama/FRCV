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

	// which backend this sink actually ended up running - RKNN vs ONNX Runtime is decided once,
	// at construction, from the uploaded model's own file format (see Model.cs's Provider field
	// and ModelManager.AddModel), not something a caller picks independently the way
	// ApriltagDetector's CPU/Vulkan choice is - there's no such thing as "the same model" in
	// both formats to switch between. This is purely informational, mirroring
	// ApriltagDetector::GetBackendName for the webui Inspector.
	std::string GetBackendName() const { return m_Backend ? m_Backend->Name() : "none"; }

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
