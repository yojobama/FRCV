#ifdef LUMEN_WITH_NT4
#include "NetworkTablesSink.h"
#include "SystemMonitor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <numbers>
#include <string_view>

namespace {
	// arbitrary but generous; a real deployment binds a handful of detector nodes, not dozens
	constexpr int MAX_BOUND_SOURCES = 16;

	// bumped whenever the binary "result" packet's own byte layout changes - a consumer
	// (photoncompat, ROADMAP.md Phase E2) reads this first and refuses to decode a payload from a
	// schema version it doesn't understand, rather than silently misreading bytes.
	constexpr uint16_t RESULT_PACKET_SCHEMA_VERSION = 1;

	void AppendU8(std::vector<uint8_t>& buf, uint8_t v) {
		buf.push_back(v);
	}
	void AppendU16(std::vector<uint8_t>& buf, uint16_t v) {
		buf.push_back(static_cast<uint8_t>(v >> 8));
		buf.push_back(static_cast<uint8_t>(v));
	}
	// big-endian regardless of host - the byte layout must not depend on this coprocessor
	// (little-endian aarch64/x86_64) and whatever eventually decodes it happening to agree.
	void AppendF32(std::vector<uint8_t>& buf, float v) {
		uint32_t bits;
		static_assert(sizeof(bits) == sizeof(v));
		std::memcpy(&bits, &v, sizeof(bits));
		for (int shift = 24; shift >= 0; shift -= 8) buf.push_back(static_cast<uint8_t>(bits >> shift));
	}
	void AppendF64(std::vector<uint8_t>& buf, double v) {
		uint64_t bits;
		static_assert(sizeof(bits) == sizeof(v));
		std::memcpy(&bits, &v, sizeof(bits));
		for (int shift = 56; shift >= 0; shift -= 8) buf.push_back(static_cast<uint8_t>(bits >> shift));
	}

	// One tag's worth of everything both the binary packet and the flattened scalar topics need,
	// computed once per tag rather than duplicated between the two publish paths below.
	struct TargetMetrics {
		int id = -1;
		double yawDeg = 0.0;
		double pitchDeg = 0.0;
		// PhotonVision's own AprilTag pipelines never actually compute a meaningful skew (that
		// field exists for the colored-shape/retroreflective pipelines PhotonVision also
		// supports) - always 0 here is matching real upstream behaviour for a fiducial-only
		// target, not a corner cut.
		double skewDeg = 0.0;
		double areaPercent = 0.0;
		bool hasPose = false;
		float tx = 0.0f, ty = 0.0f, tz = 0.0f;
		float qw = 1.0f, qx = 0.0f, qy = 0.0f, qz = 0.0f;
		// estimate_tag_pose (ApriltagDetector.cpp) runs a single-hypothesis homography solve, not
		// the dual-hypothesis IPPE PhotonVision's own ambiguity metric is derived from - there is
		// no second candidate pose here to compare against, so this is always 0 rather than a
		// fabricated number. Kept as a real field (not omitted) so the packet layout matches what
		// a future dual-hypothesis solve could actually populate.
		double poseAmbiguity = 0.0;
		std::array<float, 8> corners{}; // x0,y0,x1,y1,x2,y2,x3,y3
	};

	// Shoelace formula on the 4 detected corners - works regardless of calibration (unlike
	// yaw/pitch/pose, which need real intrinsics), so this alone is enough to pick a "best" target
	// even on an uncalibrated camera.
	double QuadPixelArea(const std::array<float, 8>& c) {
		double sum = 0.0;
		for (int i = 0; i < 4; i++) {
			int j = (i + 1) % 4;
			sum += static_cast<double>(c[i * 2]) * c[j * 2 + 1] - static_cast<double>(c[j * 2]) * c[i * 2 + 1];
		}
		return std::abs(sum) / 2.0;
	}

	// Row-major 3x3 rotation matrix (ApriltagDetector.cpp's own pose.R layout) -> unit quaternion,
	// via the standard largest-diagonal-term method (avoids the sqrt-of-a-small/negative-number
	// instability a naive formula hits when trace is small).
	void RotationMatrixToQuaternion(const nlohmann::json& r, float& qw, float& qx, float& qy, float& qz) {
		double m00 = r[0][0], m01 = r[0][1], m02 = r[0][2];
		double m10 = r[1][0], m11 = r[1][1], m12 = r[1][2];
		double m20 = r[2][0], m21 = r[2][1], m22 = r[2][2];
		double trace = m00 + m11 + m22;
		double w, x, y, z;
		if (trace > 0.0) {
			double s = std::sqrt(trace + 1.0) * 2.0;
			w = 0.25 * s;
			x = (m21 - m12) / s;
			y = (m02 - m20) / s;
			z = (m10 - m01) / s;
		} else if (m00 > m11 && m00 > m22) {
			double s = std::sqrt(1.0 + m00 - m11 - m22) * 2.0;
			w = (m21 - m12) / s;
			x = 0.25 * s;
			y = (m01 + m10) / s;
			z = (m02 + m20) / s;
		} else if (m11 > m22) {
			double s = std::sqrt(1.0 + m11 - m00 - m22) * 2.0;
			w = (m02 - m20) / s;
			x = (m01 + m10) / s;
			y = 0.25 * s;
			z = (m12 + m21) / s;
		} else {
			double s = std::sqrt(1.0 + m22 - m00 - m11) * 2.0;
			w = (m10 - m01) / s;
			x = (m02 + m20) / s;
			y = (m12 + m21) / s;
			z = 0.25 * s;
		}
		qw = static_cast<float>(w);
		qx = static_cast<float>(x);
		qy = static_cast<float>(y);
		qz = static_cast<float>(z);
	}

	std::vector<TargetMetrics> ComputeTargetMetrics(const nlohmann::json& tagsArray, const nlohmann::json& calibration) {
		bool hasFrameSize = calibration.is_object() && calibration.value("imageWidth", 0) > 0 && calibration.value("imageHeight", 0) > 0;
		double frameArea = hasFrameSize ? static_cast<double>(calibration.value("imageWidth", 0)) * calibration.value("imageHeight", 0) : 0.0;

		std::vector<TargetMetrics> out;
		out.reserve(tagsArray.size());
		for (const auto& tag : tagsArray) {
			TargetMetrics m;
			m.id = tag.value("id", -1);
			const auto& corners = tag["corners"];
			for (int i = 0; i < 4; i++) {
				m.corners[i * 2] = static_cast<float>(corners[i][0].get<double>());
				m.corners[i * 2 + 1] = static_cast<float>(corners[i][1].get<double>());
			}
			double pixelArea = QuadPixelArea(m.corners);
			m.areaPercent = hasFrameSize ? (pixelArea / frameArea) * 100.0 : 0.0;

			if (tag.contains("pose")) {
				const auto& pose = tag["pose"];
				double x = pose.value("x", 0.0), y = pose.value("y", 0.0), z = pose.value("z", 0.0);
				m.hasPose = true;
				m.tx = static_cast<float>(x);
				m.ty = static_cast<float>(y);
				m.tz = static_cast<float>(z);
				// apriltag's own camera-frame convention (also used verbatim for tags/x,y,z below):
				// +X right, +Y down, +Z forward out of the lens. Yaw is the horizontal angle off
				// boresight (positive = target to the right), pitch the vertical angle (negated Y
				// so positive = target above boresight, matching PhotonVision's own sign
				// convention for pitch) - deliberately NOT remapped into WPILib's NWU field
				// convention here, same reasoning the existing tags/r0..r8 rotation matrix publish
				// already documents: publish the raw, unambiguous camera-frame numbers and let the
				// robot-side vendordep (which already owns the WPILib Rotation3d/Transform3d
				// conversion) apply whatever convention it needs, rather than risking a
				// server-side convention bug no robot-side code could detect.
				m.yawDeg = std::atan2(x, z) * 180.0 / std::numbers::pi;
				m.pitchDeg = std::atan2(-y, z) * 180.0 / std::numbers::pi;
				static const std::array<double, 3> identityRow{ 0.0, 0.0, 0.0 };
				auto rotation = pose.value("R", nlohmann::json::array({identityRow, identityRow, identityRow}));
				RotationMatrixToQuaternion(rotation, m.qw, m.qx, m.qy, m.qz);
			}
			out.push_back(m);
		}
		return out;
	}

	// [u16 schemaVersion][u8 targetCount][repeated per target: u16 fiducialId, f64 yaw, f64 pitch,
	// f64 area, f64 skew, f32x3 translation, f32x4 rotation-quaternion(w,x,y,z), f64 poseAmbiguity,
	// f32x8 corners] - all multi-byte fields big-endian (see AppendU16/F32/F64 above). Mirrors
	// photonlib's own hand-packed Packet approach: results are variable-length per frame, and
	// WPILib's fixed-size Struct serialization has no way to express that. There is no shared IDL
	// between this C++ side and the eventual Java decoder (ROADMAP.md Phase E2/photoncompat) -
	// this comment IS the spec both sides have to agree with by hand.
	std::vector<uint8_t> BuildResultPacket(const std::vector<TargetMetrics>& targets) {
		std::vector<uint8_t> packet;
		AppendU16(packet, RESULT_PACKET_SCHEMA_VERSION);
		AppendU8(packet, static_cast<uint8_t>(std::min<size_t>(targets.size(), 255)));
		for (size_t i = 0; i < targets.size() && i < 255; i++) {
			const TargetMetrics& t = targets[i];
			AppendU16(packet, static_cast<uint16_t>(t.id));
			AppendF64(packet, t.yawDeg);
			AppendF64(packet, t.pitchDeg);
			AppendF64(packet, t.areaPercent);
			AppendF64(packet, t.skewDeg);
			AppendF32(packet, t.tx);
			AppendF32(packet, t.ty);
			AppendF32(packet, t.tz);
			AppendF32(packet, t.qw);
			AppendF32(packet, t.qx);
			AppendF32(packet, t.qy);
			AppendF32(packet, t.qz);
			AppendF64(packet, t.poseAmbiguity);
			for (float c : t.corners) AppendF32(packet, c);
		}
		return packet;
	}
}

NetworkTablesSink::NetworkTablesSink(std::shared_ptr<Logger> logger, std::string id, NetworkTablesConfig config, SystemMonitor* systemMonitor)
	: ISink(logger, MAX_BOUND_SOURCES, true /* requireJson */, false /* requireFrame */, id)
	, m_Instance(nt::NetworkTableInstance::Create())
	, m_Config(config)
	, m_Logger(logger)
	, m_SystemMonitor(systemMonitor)
	, m_ConstructedAtUs(SourceResult::NowUs())
{
	if (m_Logger) m_Logger->EnterLog("NetworkTablesSink constructed, identity=" + config.clientIdentity);

	if (config.teamNumber.has_value()) {
		m_Instance.SetServerTeam(config.teamNumber.value(), config.port);
	} else {
		m_Instance.SetServer(config.serverAddress, config.port);
	}
	m_Instance.StartClient4(config.clientIdentity);

	// prefix-subscribed to this sink's own whole subtree (not per-bound-source - ISink has no
	// hook for "a source just got bound/unbound" this could piggyback on, and a robot writing to
	// an id nothing is currently bound to is just a harmless no-op once polled) so a robot can
	// write "<rootTable>/<sourceId>/config/pipelineIndex" or ".../driverMode" for any source this
	// sink ever publishes, present or not yet bound.
	// leading slash required - every real topic name is absolute ("/lumenvision/...", confirmed
	// against this sink's own published topics), and a prefix without it matches nothing at all
	// (confirmed the hard way: the listener never fired once, silently, with no error).
	std::string configPrefix = "/" + m_Config.rootTable + "/";
	std::array<std::string_view, 1> prefixes{ std::string_view(configPrefix) };
	m_ConfigListener = m_Instance.AddListener(prefixes, NT_EVENT_VALUE_REMOTE,
		[this](const nt::Event& event) { OnConfigValueChanged(event); });

	m_RecordingStatusPublisher = m_Instance.GetBooleanTopic("/" + m_Config.rootTable + "/status/recording").Publish();
	m_RecordingStatusPublisher.Set(false);
}

NetworkTablesSink::~NetworkTablesSink()
{
	m_Instance.RemoveListener(m_ConfigListener);
	m_Instance.StopClient();
}

void NetworkTablesSink::OnConfigValueChanged(const nt::Event& event)
{
	const nt::ValueEventData* valueData = event.GetValueEventData();
	if (valueData == nullptr) return;

	// "/<rootTable>/<sourceId>/config/pipelineIndex" or ".../driverMode" - GetTopicName always
	// returns a leading-slash absolute name (confirmed against every other topic this sink itself
	// publishes), so the expected prefix below includes it too.
	std::string name = nt::GetTopicName(valueData->topic);
	std::string prefix = "/" + m_Config.rootTable + "/";
	if (name.rfind(prefix, 0) != 0) return;
	std::string rest = name.substr(prefix.size());

	// coprocessor-wide, not per-source: "/<rootTable>/config/recording" - the per-source parse
	// below requires exactly "<id>/config/<leaf>" and would drop it
	if (rest == "config/recording") {
		if (valueData->value.IsBoolean()) {
			std::lock_guard<std::mutex> lock(m_ConfigMutex);
			m_PendingRecording = valueData->value.GetBoolean();
		}
		return;
	}

	size_t firstSlash = rest.find('/');
	size_t secondSlash = rest.find('/', firstSlash == std::string::npos ? std::string::npos : firstSlash + 1);
	if (firstSlash == std::string::npos || secondSlash == std::string::npos) return;
	if (rest.substr(firstSlash + 1, secondSlash - firstSlash - 1) != "config") return;

	std::string sourceId = rest.substr(0, firstSlash);
	std::string leaf = rest.substr(secondSlash + 1);

	std::lock_guard<std::mutex> lock(m_ConfigMutex);
	if (leaf == "pipelineIndex" && (valueData->value.IsInteger() || valueData->value.IsDouble())) {
		m_PendingConfig[sourceId].pipelineIndex = valueData->value.IsInteger()
			? static_cast<int>(valueData->value.GetInteger())
			: static_cast<int>(valueData->value.GetDouble());
	} else if (leaf == "driverMode" && valueData->value.IsBoolean()) {
		m_PendingConfig[sourceId].driverMode = valueData->value.GetBoolean();
	}
}

std::string NetworkTablesSink::PollConfigRequests()
{
	std::unordered_map<std::string, PendingConfigRequest> drained;
	{
		std::lock_guard<std::mutex> lock(m_ConfigMutex);
		drained.swap(m_PendingConfig);
	}

	nlohmann::json out = nlohmann::json::array();
	for (const auto& [sourceId, request] : drained) {
		nlohmann::json entry{ {"sourceId", sourceId} };
		if (request.pipelineIndex.has_value()) entry["pipelineIndex"] = request.pipelineIndex.value();
		if (request.driverMode.has_value()) entry["driverMode"] = request.driverMode.value();
		out.push_back(entry);
	}
	return out.dump();
}

int NetworkTablesSink::PollRecordingRequest()
{
	std::lock_guard<std::mutex> lock(m_ConfigMutex);
	if (!m_PendingRecording.has_value()) return -1;
	int value = m_PendingRecording.value() ? 1 : 0;
	m_PendingRecording.reset();
	return value;
}

void NetworkTablesSink::SetRecordingStatus(bool recording)
{
	m_RecordingStatusPublisher.Set(recording);
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

void NetworkTablesSink::PublishSourceResult(const SourceResult& result)
{
	const std::string& sourceId = result.sourceId;
	const nlohmann::json& json = result.json.value();
	auto table = m_Instance.GetTable(m_Config.rootTable + "/" + sourceId);

	// AprilTag detector shape: either a bare array of {id, center, corners, pose:{x,y,z,R}}
	// objects (the original shape, still what e.g. ObjectDetectionSink's differently-shaped
	// array falls through past below), or - since ROADMAP.md Phase 7's multi-tag PnP - an
	// object envelope {"tags": [...same per-tag shape...], "multiTag": {...} | null,
	// "calibration": {...} | null}, ApriltagDetector's own current shape. Detect both
	// structurally rather than trusting a "type" field the JSON doesn't carry today.
	nlohmann::json tagsArray;
	nlohmann::json calibration = nullptr;
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
		calibration = json.value("calibration", nlohmann::json(nullptr));
	}

	if (looksLikeTags) {
		std::vector<double> ids, x, y, z;
		// row-major 3x3 rotation matrix, flattened into 9 parallel arrays (r0..r8, matching
		// ApriltagDetector.cpp's own R=[[r0,r1,r2],[r3,r4,r5],[r6,r7,r8]] layout) rather than a
		// quaternion or Euler angles: NT4's NumberArray type has no nested-array support, and
		// publishing the raw matrix (not a derived representation computed here) is what lets
		// WPILib's own Rotation3d(Matrix<N3, N3>) constructor build a Transform3d robot-side
		// with no conversion-convention bug this end could introduce - see ApriltagDetector.cpp
		// for the full rationale (ROADMAP.md Phase 7's blocking prerequisite). Kept alongside the
		// newer flattened/binary publishes below rather than replaced - AdvantageScope graphing
		// of a raw per-tag array is still useful and nothing downstream depended on removing it.
		std::array<std::vector<double>, 9> r;
		ids.reserve(tagsArray.size());
		x.reserve(tagsArray.size());
		y.reserve(tagsArray.size());
		z.reserve(tagsArray.size());
		for (auto& ri : r) ri.reserve(tagsArray.size());

		for (const auto& tag : tagsArray) {
			ids.push_back(tag.value("id", -1));
			// "pose" is only present when ApriltagDetector had real calibration attached
			// (ApriltagDetector.cpp only sets it under m_HasCalibration) - an uncalibrated
			// detector's tags still publish ids/corners, just no x/y/z/R (left at the same
			// zeroed default the missing-multiTag/missing-rotation branches elsewhere in this
			// function already use), rather than indexing a key that may not exist.
			nlohmann::json pose = tag.value("pose", nlohmann::json::object());
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

		// --- flattened best-target scalars + versioned binary packet (ROADMAP.md Phase E1) ---
		std::vector<TargetMetrics> targets = ComputeTargetMetrics(tagsArray, calibration);
		const TargetMetrics* best = nullptr;
		for (const auto& t : targets) {
			if (best == nullptr || t.areaPercent > best->areaPercent) best = &t;
		}

		table->PutBoolean("hasTargets", best != nullptr);
		table->PutNumber("targetYaw", best ? best->yawDeg : 0.0);
		table->PutNumber("targetPitch", best ? best->pitchDeg : 0.0);
		table->PutNumber("targetArea", best ? best->areaPercent : 0.0);
		// [x, y, z, qw, qx, qy, qz] - translation + unit quaternion, not NT4 struct:Transform3d:
		// no wpi::Struct<Transform3d> specialization exists on this C++ side (nothing in this
		// codebase publishes WPILib struct-schema topics yet), and a flat double array needs no
		// new machinery to get right - photoncompat (Phase E2) can build a real Transform3d from
		// these 7 numbers with one WPILib constructor call.
		table->PutNumberArray("targetPose", best
			? std::vector<double>{best->tx, best->ty, best->tz, best->qw, best->qx, best->qy, best->qz}
			: std::vector<double>{0, 0, 0, 1, 0, 0, 0});

		if (calibration.is_object()) {
			double fx = calibration.value("fx", 0.0), fy = calibration.value("fy", 0.0);
			double cx = calibration.value("cx", 0.0), cy = calibration.value("cy", 0.0);
			table->PutNumberArray("cameraIntrinsics", std::vector<double>{fx, 0, cx, 0, fy, cy, 0, 0, 1});
			table->PutNumberArray("cameraDistortion", calibration.value("distCoeffs", std::vector<double>{}));
		}

		table->PutRaw("result", BuildResultPacket(targets));
	} else {
		table->PutString("raw", json.dump());
	}

	// pipeline latency (capture -> published-to-NT), independent of NT4's own network-layer
	// timestamping - both captureTimeUs/producedTimeUs are drawn from the same clock
	// (SourceResult::NowUs(), see its own comment), so this is a real measurement even before
	// accounting for anything NT4-specific.
	double latencyMs = (result.producedTimeUs > result.captureTimeUs)
		? static_cast<double>(result.producedTimeUs - result.captureTimeUs) / 1000.0
		: 0.0;
	table->PutNumber("latencyMs", latencyMs);

	NetworkTablesStreamStats& stats = m_StreamStats[sourceId];
	if (stats.lastCaptureTimeUs != 0 && result.captureTimeUs > stats.lastCaptureTimeUs && result.frameNumber > stats.lastFrameNumber) {
		double deltaSeconds = static_cast<double>(result.captureTimeUs - stats.lastCaptureTimeUs) / 1'000'000.0;
		double deltaFrames = static_cast<double>(result.frameNumber - stats.lastFrameNumber);
		if (deltaSeconds > 0.0) stats.fps = deltaFrames / deltaSeconds;
	}
	stats.lastCaptureTimeUs = result.captureTimeUs;
	stats.lastFrameNumber = result.frameNumber;
	table->PutNumber("fps", stats.fps);
}

void NetworkTablesSink::Process(const std::vector<SourceResult>& results)
{
	for (const SourceResult& result : results) {
		if (!result.json.has_value()) continue;
		PublishSourceResult(result);
	}

	auto rootTable = m_Instance.GetTable(m_Config.rootTable);
	rootTable->PutNumber("heartbeat", static_cast<double>(m_Heartbeat++));

	// re-published every tick, not just once at construction: a value Set() before this sink's
	// own NT4 client has completed its first connection to the server doesn't reliably reach a
	// server it wasn't yet connected to (confirmed the hard way writing this sink's own NT4 e2e
	// test - a one-shot ".version" publish in the constructor never arrived, a per-tick one
	// always does, same as heartbeat/.status already being per-tick). Cheap enough (one string,
	// same rate as heartbeat) that there's no reason to special-case it back to one-shot.
	rootTable->PutString(".version", LUMEN_VERSION_STRING);

	// server time offset (ntcore's own NT4 time-sync measurement, nt::GetServerTimeOffset) - not
	// re-derived here, just surfaced: a bad/absent offset is exactly what a robot program needs
	// to see to know its own addVisionMeasurement() timestamps can't be trusted yet.
	std::optional<int64_t> serverTimeOffsetUs = m_Instance.GetServerTimeOffset();
	nlohmann::json statusJson{
		{"uptimeSeconds", static_cast<double>(SourceResult::NowUs() - m_ConstructedAtUs) / 1'000'000.0},
		{"nodeCount", static_cast<int>(results.size())},
		{"serverTimeOffsetUs", serverTimeOffsetUs.has_value() ? nlohmann::json(serverTimeOffsetUs.value()) : nlohmann::json(nullptr)},
	};
	if (m_SystemMonitor != nullptr) {
		statusJson["cpuUsage"] = m_SystemMonitor->GetCPUUsage();
		statusJson["cpuTemperature"] = m_SystemMonitor->GetCPUTemperature();
		statusJson["ramUsage"] = m_SystemMonitor->GetRAMUsage();
	}
	rootTable->PutString(".status", statusJson.dump());
}

#endif // LUMEN_WITH_NT4
