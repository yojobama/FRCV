#ifdef LUMEN_WITH_NT4
#include "NetworkTablesSink.h"
#include <array>

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

	// AprilTag detector shape: either a bare array of {id, center, corners, pose:{x,y,z,R}}
	// objects (the original shape, still what e.g. ObjectDetectionSink's differently-shaped
	// array falls through past below), or - since ROADMAP.md Phase 7's multi-tag PnP - an
	// object envelope {"tags": [...same per-tag shape...], "multiTag": {...} | null},
	// ApriltagDetector's own current shape. Detect both structurally rather than trusting a
	// "type" field the JSON doesn't carry today.
	nlohmann::json tagsArray;
	bool looksLikeTags = false;
	bool hasEnvelope = false;
	if (json.is_array()) {
		// An EMPTY array counts too - not just non-empty arrays shaped like a tag - or a tag
		// leaving frame would leave tags/ids (and x/y/z/r0..r8) at their last stale published
		// values forever instead of updating to "zero tags this frame", which a robot pose
		// estimator reading this table has no way to distinguish from "still seeing that tag".
		looksLikeTags = json.empty() || (json[0].is_object() && json[0].contains("id") && json[0].contains("pose"));
		tagsArray = json;
	} else if (json.is_object() && json.contains("tags") && json["tags"].is_array()) {
		looksLikeTags = true;
		hasEnvelope = true;
		tagsArray = json["tags"];
	}

	if (looksLikeTags) {
		std::vector<double> ids, x, y, z;
		// row-major 3x3 rotation matrix, flattened into 9 parallel arrays (r0..r8, matching
		// ApriltagDetector.cpp's own R=[[r0,r1,r2],[r3,r4,r5],[r6,r7,r8]] layout) rather than a
		// quaternion or Euler angles: NT4's NumberArray type has no nested-array support, and
		// publishing the raw matrix (not a derived representation computed here) is what lets
		// WPILib's own Rotation3d(Matrix<N3, N3>) constructor build a Transform3d robot-side
		// with no conversion-convention bug this end could introduce - see ApriltagDetector.cpp
		// for the full rationale (ROADMAP.md Phase 7's blocking prerequisite).
		std::array<std::vector<double>, 9> r;
		ids.reserve(tagsArray.size());
		x.reserve(tagsArray.size());
		y.reserve(tagsArray.size());
		z.reserve(tagsArray.size());
		for (auto& ri : r) ri.reserve(tagsArray.size());

		for (const auto& tag : tagsArray) {
			ids.push_back(tag.value("id", -1));
			const auto& pose = tag["pose"];
			x.push_back(pose.value("x", 0.0));
			y.push_back(pose.value("y", 0.0));
			z.push_back(pose.value("z", 0.0));

			static const std::array<double, 3> identityRow{ 0.0, 0.0, 0.0 };
			auto rotation = pose.value("R", nlohmann::json::array({identityRow, identityRow, identityRow}));
			for (int row = 0; row < 3; row++) {
				for (int col = 0; col < 3; col++) {
					r[row * 3 + col].push_back(rotation[row][col].get<double>());
				}
			}
		}

		table->PutNumberArray("tags/ids", ids);
		table->PutNumberArray("tags/x", x);
		table->PutNumberArray("tags/y", y);
		table->PutNumberArray("tags/z", z);
		for (int i = 0; i < 9; i++) {
			table->PutNumberArray("tags/r" + std::to_string(i), r[i]);
		}

		// multi-tag PnP result (ROADMAP.md Phase 7) - only present in the object-envelope shape.
		// Published as scalars, not parallel arrays (there's only ever one result per frame, not
		// one per tag) - x/y/z/r0..r8 are the CAMERA's own pose in FIELD frame (not
		// camera-to-tag, unlike tags/x,y,z above), matching what a robot program actually wants:
		// "where am I", not "where is this specific tag relative to me".
		nlohmann::json multiTag = hasEnvelope ? json.value("multiTag", nlohmann::json(nullptr)) : nlohmann::json(nullptr);
		if (!multiTag.is_null()) {
			table->PutNumber("multitag/x", multiTag.value("x", 0.0));
			table->PutNumber("multitag/y", multiTag.value("y", 0.0));
			table->PutNumber("multitag/z", multiTag.value("z", 0.0));
			static const std::array<double, 3> identityRow{ 0.0, 0.0, 0.0 };
			auto rotation = multiTag.value("R", nlohmann::json::array({identityRow, identityRow, identityRow}));
			for (int row = 0; row < 3; row++) {
				for (int col = 0; col < 3; col++) {
					table->PutNumber("multitag/r" + std::to_string(row * 3 + col), rotation[row][col].get<double>());
				}
			}
			table->PutNumber("multitag/tagCount", multiTag.value("tagCount", 0));
			table->PutNumber("multitag/reprojErrPixels", multiTag.value("reprojErrPixels", 0.0));
		} else {
			// no multi-tag result this frame (fewer than 2 known-field-pose tags visible, no
			// field layout loaded, or this sink predates Phase 7 and never sends the envelope) -
			// clear tagCount to 0 rather than leaving a stale prior result/pose published, same
			// reasoning as tags/* above.
			table->PutNumber("multitag/tagCount", 0);
		}
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
