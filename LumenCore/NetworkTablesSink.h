#pragma once
#ifdef FRCV_WITH_NT4

#include "ISink.h"
#include <networktables/NetworkTableInstance.h>
#include <optional>
#include <string>

struct NetworkTablesConfig {
	// exactly one of these should be set; teamNumber takes precedence if both are
	std::optional<unsigned int> teamNumber;
	std::string serverAddress; // used when teamNumber is not set
	unsigned int port = 0;     // 0 = NT4 default port
	std::string clientIdentity = "FRCV";
	std::string rootTable = "FRCV";
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
	NetworkTablesSink(std::shared_ptr<Logger> logger, std::string id, NetworkTablesConfig config);
	~NetworkTablesSink();

	bool IsConnected() const;
	// small JSON status blob (connected, server, identity, latency) for Manager::GetSinkResult-
	// style introspection; NetworkTablesSink has no ISource half of its own to hang this off, so
	// it is exposed as a plain method instead
	std::string GetConnectionStatus() const;

private:
	void Process(std::vector<SourceResult> results) override;

	// interprets one source's JSON and writes it into that source's NT subtable. Recognizes the
	// AprilTag detector's array-of-detections shape (id/center/corners/pose) and publishes the
	// tag ids and pose translation as parallel arrays (tags/ids, tags/x, tags/y, tags/z);
	// anything else - including today's object-detection stub and any shape not recognized -
	// falls back to a single "raw" string topic with the JSON as-is, so nothing bound to this
	// sink is ever silently dropped even before its specific NT mapping is written.
	void PublishSourceResult(const std::string& sourceId, const nlohmann::json& json);

	nt::NetworkTableInstance m_Instance;
	NetworkTablesConfig m_Config;
	std::shared_ptr<Logger> m_Logger;
	uint64_t m_Heartbeat = 0;
};

#endif // FRCV_WITH_NT4
