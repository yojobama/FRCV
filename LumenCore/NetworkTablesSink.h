#pragma once
#ifdef LUMEN_WITH_NT4

#include "ISink.h"
#include <networktables/NetworkTableInstance.h>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class SystemMonitor;

struct NetworkTablesConfig {
	// exactly one of these should be set; teamNumber takes precedence if both are
	std::optional<unsigned int> teamNumber;
	std::string serverAddress; // used when teamNumber is not set
	unsigned int port = 0;     // 0 = NT4 default port
	// independent literals, not one copied into the other: sharing a single default (as this
	// struct used to) means two coprocessors on one robot silently register the same NT4 client
	// identity if neither is customized. A real per-device default (hostname-derived, or similar)
	// is a vendordep-phase concern, not a rename - these are the same plain defaults the REST
	// controllers already pass explicitly, so in practice this struct's own initializers are only
	// reached by a caller that default-constructs NetworkTablesConfig directly.
	std::string clientIdentity = "lumenvision";
	std::string rootTable = "lumenvision";
};

// One bound source's rolling stream stats, tracked across Process() calls so fps can be derived
// (a single SourceResult only carries enough to compute per-frame latency, not a rate).
struct NetworkTablesStreamStats {
	uint64_t lastFrameNumber = 0;
	uint64_t lastCaptureTimeUs = 0;
	double fps = 0.0;
};

// Terminal sink (produces nothing, so it is never also registered as an ISource): publishes
// every bound source's latest JSON result onto an NT4 server, one subtable per source keyed by
// that source's id. This is deliberately generic rather than AprilTag-specific, since it will
// also carry object detection results (phase 4) - PublishSourceResult below is where a result's
// shape gets interpreted, and is the place to extend when a new sink type's JSON shape needs
// dedicated NT topics rather than the raw-JSON fallback.
class NetworkTablesSink : public ISink
{
public:
	// systemMonitor is a non-owning pointer to Manager's own single SystemMonitor instance
	// (Manager::m_SystemMonitor) - sharing it rather than each NetworkTablesSink spinning up its
	// own polling thread. May be null (e.g. constructed directly in a test, outside Manager),
	// in which case ".status" simply omits the cpu/temperature/ram fields rather than crashing.
	// Manager always outlives every sink in m_Sinks, so this pointer's lifetime is safe for as
	// long as this object exists - the same ownership assumption GetTable()'s returned
	// shared_ptr<NetworkTable> and m_Logger already rely on.
	NetworkTablesSink(std::shared_ptr<Logger> logger, std::string id, NetworkTablesConfig config,
		SystemMonitor* systemMonitor = nullptr);
	~NetworkTablesSink();

	bool IsConnected() const;
	// small JSON status blob (connected, server, identity, latency) for Manager::GetSinkResult-
	// style introspection; NetworkTablesSink has no ISource half of its own to hang this off, so
	// it is exposed as a plain method instead
	std::string GetConnectionStatus() const;

	// Drains every robot-writable "<sourceId>/config/pipelineIndex" and
	// "<sourceId>/config/driverMode" write seen since the last call, as a JSON array of
	// {"sourceId": ..., "pipelineIndex": int|omitted, "driverMode": bool|omitted}. Consuming
	// (not just reading) is deliberate: Manager/the C# Server side is expected to poll this on its
	// own existing periodic loop and apply each request exactly once via its own SetDriverMode/
	// pipeline-profile-activation logic - this sink has no way to call into that logic directly
	// (ISink/ISource classes never reach back into Manager), so it can only surface the request,
	// not apply it. This is a real gap for now: nothing on the C# side polls this yet.
	std::string PollConfigRequests();

private:
	// nt::Event callback registered on m_Instance covering every topic under this sink's own
	// root table - parses "<rootTable>/<sourceId>/config/pipelineIndex" and ".../driverMode"
	// writes out of the raw topic name (there is no cheaper way to know which bound source a
	// remote write was meant for: NT4 topics are flat strings, and ISink has no per-source
	// listener hook to piggyback on).
	void OnConfigValueChanged(const nt::Event& event);
	void Process(const std::vector<SourceResult>& results) override;

	// interprets one source's JSON and writes it into that source's NT subtable: the legacy
	// parallel-array shape (tags/ids, tags/x, ...), the new flattened best-target scalar topics
	// (hasTargets/targetYaw/...), and the versioned binary "result" packet (see
	// BuildResultPacket's own comment for the exact layout) all side by side - AdvantageScope/
	// Shuffleboard can graph the flattened scalars with no decoding, while photoncompat
	// (ROADMAP.md Phase E1/E2) decodes the packet for the full per-target detail none of the
	// scalar topics carry alone (corners, quaternion, ambiguity). Anything that doesn't look like
	// an AprilTag detector's shape - including today's object-detection stub - falls back to a
	// single "raw" string topic with the JSON as-is, so nothing bound to this sink is ever
	// silently dropped even before its specific NT mapping is written.
	void PublishSourceResult(const SourceResult& result);

	nt::NetworkTableInstance m_Instance;
	NetworkTablesConfig m_Config;
	std::shared_ptr<Logger> m_Logger;
	SystemMonitor* m_SystemMonitor;
	uint64_t m_Heartbeat = 0;
	uint64_t m_ConstructedAtUs = 0;
	std::unordered_map<std::string, NetworkTablesStreamStats> m_StreamStats;

	NT_Listener m_ConfigListener = 0;
	std::mutex m_ConfigMutex;
	struct PendingConfigRequest {
		std::optional<int> pipelineIndex;
		std::optional<bool> driverMode;
	};
	// guarded by m_ConfigMutex - OnConfigValueChanged runs on ntcore's own listener thread, not
	// this sink's own Process() thread.
	std::unordered_map<std::string, PendingConfigRequest> m_PendingConfig;
};

#endif // LUMEN_WITH_NT4
