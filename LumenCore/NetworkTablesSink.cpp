#ifdef LUMEN_WITH_NT4
#include "NetworkTablesSink.h"

namespace {
	// arbitrary but generous; a real deployment binds a handful of detector nodes, not dozens
	constexpr int MAX_BOUND_SOURCES = 16;
}

NetworkTablesSink::NetworkTablesSink(std::shared_ptr<Logger> logger, std::string id, NetworkTablesConfig config)
	: ISink(logger, MAX_BOUND_SOURCES, true /* requireJson */, false /* requireFrame */, id)
	, m_Instance(nt::NetworkTableInstance::Create())
	, m_Config(config)
	, m_Logger(logger)
{
	if (m_Logger) m_Logger->EnterLog("NetworkTablesSink constructed, identity=" + config.clientIdentity);

	if (config.teamNumber.has_value()) {
		m_Instance.SetServerTeam(config.teamNumber.value(), config.port);
	} else {
		m_Instance.SetServer(config.serverAddress, config.port);
	}
	m_Instance.StartClient4(config.clientIdentity);
}

NetworkTablesSink::~NetworkTablesSink()
{
	m_Instance.StopClient();
}

bool NetworkTablesSink::IsConnected() const
{
	return m_Instance.IsConnected();
}

std::string NetworkTablesSink::GetConnectionStatus() const
{
	nlohmann::json status{
		{"connected", IsConnected()},
		{"identity", m_Config.clientIdentity},
		{"rootTable", m_Config.rootTable},
	};
	if (m_Config.teamNumber.has_value()) {
		status["teamNumber"] = m_Config.teamNumber.value();
	} else {
		status["serverAddress"] = m_Config.serverAddress;
	}
	return status.dump();
}

void NetworkTablesSink::PublishSourceResult(const std::string& sourceId, const nlohmann::json& json)
{
	auto table = m_Instance.GetTable(m_Config.rootTable + "/" + sourceId);

	// AprilTag detector shape: an array of {id, center, corners, pose:{x,y,z}} objects.
	// Detect it structurally rather than trusting a "type" field the JSON doesn't carry today.
	bool looksLikeTags = json.is_array() && !json.empty() && json[0].is_object()
		&& json[0].contains("id") && json[0].contains("pose");

	if (looksLikeTags) {
		std::vector<double> ids, x, y, z;
		ids.reserve(json.size());
		x.reserve(json.size());
		y.reserve(json.size());
		z.reserve(json.size());

		for (const auto& tag : json) {
			ids.push_back(tag.value("id", -1));
			const auto& pose = tag["pose"];
			x.push_back(pose.value("x", 0.0));
			y.push_back(pose.value("y", 0.0));
			z.push_back(pose.value("z", 0.0));
		}

		table->PutNumberArray("tags/ids", ids);
		table->PutNumberArray("tags/x", x);
		table->PutNumberArray("tags/y", y);
		table->PutNumberArray("tags/z", z);
	} else {
		table->PutString("raw", json.dump());
	}
}

void NetworkTablesSink::Process(std::vector<SourceResult> results)
{
	for (const SourceResult& result : results) {
		if (!result.json.has_value()) continue;
		PublishSourceResult(result.sourceId, result.json.value());
	}

	auto rootTable = m_Instance.GetTable(m_Config.rootTable);
	rootTable->PutNumber("heartbeat", static_cast<double>(m_Heartbeat++));
}

#endif // LUMEN_WITH_NT4
