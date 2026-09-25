#include <catch2/catch_test_macros.hpp>
#include "ApriltagDetector.h"
#include "CameraCalibrationResult.h"
#include "NetworkTablesSink.h"
#include <chrono>
#include <thread>

// ROADMAP.md Phase E1: an actual end-to-end value check of the real (not soon-to-be-replaced)
// NT4 schema, deliberately written after NetworkTablesSink's schema rework rather than against
// the old parallel-array-only shape. ntcore's own standard local-testing pattern
// (nt::NetworkTableInstance::Create() + StartServer() on a loopback instance, a second instance
// as the client - see NetworkTableInstance.h's own StartServer/SetServer docs) needs no real
// network and no Pi - just two local ntcore instances talking over 127.0.0.1.
//
// ImageFileSource -> ApriltagDetector -> NetworkTablesSink, against the same real,
// already-hardware-verified fixture the CPU-vs-Vulkan agreement test uses (grayimage.pgm from the
// vkapriltag submodule) - no calibration attached, so this checks the parts of the new schema
// that don't depend on pose estimation (hasTargets/tags-ids/the binary packet's targetCount/
// .version/.status/heartbeat), not yaw/pitch/targetPose (already covered separately: the actual
// yaw/pitch math is plain trigonometry, not worth a second real-detector round trip to verify).

namespace {
class PgmFrameSource : public ISource {
public:
	PgmFrameSource(std::shared_ptr<Logger> logger, std::string id, cv::Mat bgrFrame)
		: ISource(logger, id), m_Frame(std::move(bgrFrame))
	{
	}

protected:
	void CaptureFrame() override {
		SetLatestResult(SourceResult(std::nullopt, m_Frame.clone()));
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

private:
	cv::Mat m_Frame;
};

// polls rather than sleeping a fixed duration - ntcore's own client/server handshake plus at
// least one full capture->detect->publish cycle has no fixed upper bound worth hardcoding.
// Confirmed the hard way that a single fixed sleep before checking is genuinely flaky here (a
// value present after 300ms on one run took noticeably longer on another) - a bounded poll is
// both faster on the common case and reliable on a slower CI runner.
template<typename Predicate>
bool WaitUntil(Predicate predicate, int maxAttempts = 100, int delayMs = 50) {
	for (int attempt = 0; attempt < maxAttempts; attempt++) {
		if (predicate()) return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
	}
	return predicate();
}
}

TEST_CASE("NetworkTablesSink publishes the real NT4 schema end to end over a loopback server", "[nt4][e2e]") {
	const std::string pgmPath = std::string(LUMEN_VKAPRILTAG_SAMPLE_DIR) + "/grayimage.pgm";
	cv::Mat gray = cv::imread(pgmPath, cv::IMREAD_GRAYSCALE);
	REQUIRE_FALSE(gray.empty());
	cv::Mat bgr;
	cv::cvtColor(gray, bgr, cv::COLOR_GRAY2BGR);

	// isolated, non-default ports and no persistence file (empty persist_filename) - this must
	// never touch the real NT3/NT4 default ports (1735/5810) or leave a networktables.json
	// behind in whatever directory ctest happens to run from. NT3 and NT4 need genuinely
	// DIFFERENT port numbers - StartServer binds a separate listening socket for each, and
	// passing the same value for both makes the NT4 bind fail silently ("address already in
	// use", confirmed the hard way) since NT3's listener grabs it first, leaving every NT4
	// client's handshake landing on the wrong protocol's socket.
	constexpr unsigned int TEST_NT3_PORT = 17809;
	constexpr unsigned int TEST_NT4_PORT = 17810;

	nt::NetworkTableInstance server = nt::NetworkTableInstance::Create();
	server.StartServer("", "127.0.0.1", TEST_NT3_PORT, TEST_NT4_PORT);

	auto logger = std::make_shared<Logger>("LumenCoreTests-nt4-e2e.log");
	auto imageSource = std::make_shared<PgmFrameSource>(logger, "nt4-e2e-image", bgr);
	auto detector = std::make_shared<ApriltagDetector>(logger, "nt4-e2e-detector", CameraCalibrationResult(), 0.1651);
	REQUIRE(detector->BindSource(imageSource));

	NetworkTablesConfig config;
	config.serverAddress = "127.0.0.1";
	config.port = TEST_NT4_PORT;
	config.rootTable = "lumenvision";
	config.clientIdentity = "LumenCoreTests-nt4-e2e";
	auto ntSink = std::make_shared<NetworkTablesSink>(logger, "nt4-e2e-sink", config);
	REQUIRE(ntSink->BindSource(detector));

	imageSource->Toggle(true);
	static_cast<ISink&>(*detector).Toggle(true);
	static_cast<ISink&>(*ntSink).Toggle(true);

	nt::NetworkTableInstance client = nt::NetworkTableInstance::Create();
	client.SetServer("127.0.0.1", TEST_NT4_PORT);
	client.StartClient4("LumenCoreTests-nt4-e2e-reader");
	auto clientTable = client.GetTable("lumenvision");
	auto detectorTable = clientTable->GetSubTable("nt4-e2e-detector");

	REQUIRE(WaitUntil([&] { return clientTable->GetNumber("heartbeat", -1.0) >= 0.0; }));
	REQUIRE(WaitUntil([&] { return clientTable->GetString(".version", "") == LUMEN_VERSION_STRING; }));
	REQUIRE(WaitUntil([&] { return detectorTable->GetBoolean("hasTargets", false); })); // grayimage.pgm has a real detectable tag
	REQUIRE(WaitUntil([&] { return !clientTable->GetString(".status", "").empty(); }));
	// NT4 gives no cross-topic ordering guarantee - hasTargets (a later-published bool) arriving
	// before tags/ids (an earlier-published, larger NumberArray) is a real, benign race between
	// independent topics, not a publish-order bug (confirmed the hard way: hasTargets consistently
	// visible while tags/ids still read back empty on the very next line, with no wait of its own).
	REQUIRE(WaitUntil([&] { return !detectorTable->GetNumberArray("tags/ids", std::vector<double>{}).empty(); }));
	// same benign cross-topic race as above - "result" (the binary packet) is the LAST thing
	// PublishSourceResult writes per source, so it's the one most likely to still be in flight
	// even after tags/ids is already visible.
	REQUIRE(WaitUntil([&] { return detectorTable->GetRaw("result", std::vector<uint8_t>{}).size() >= 3; }));
	// same "no cross-topic ordering" reasoning as above, but for an entirely different cause: this
	// is the FIRST access to "latencyMs" by this client, and a brand-new subscription's very first
	// sync of its topic's current value is itself not instantaneous (confirmed the hard way: a
	// one-shot read immediately after Toggle(false) consistently missed it, while every topic
	// already queried at least once above - by an earlier WaitUntil - was reliably present).
	REQUIRE(WaitUntil([&] { return detectorTable->GetNumber("latencyMs", -1.0) >= 0.0; }));

	imageSource->Toggle(false);
	static_cast<ISink&>(*detector).Toggle(false);
	static_cast<ISink&>(*ntSink).Toggle(false);

	std::vector<double> ids = detectorTable->GetNumberArray("tags/ids", std::vector<double>{});
	REQUIRE_FALSE(ids.empty());
	REQUIRE(detectorTable->GetNumber("latencyMs", -1.0) >= 0.0);

	std::vector<uint8_t> packet = detectorTable->GetRaw("result", std::vector<uint8_t>{});
	REQUIRE(packet.size() >= 3); // schemaVersion (u16) + targetCount (u8) at minimum
	uint16_t schemaVersion = (static_cast<uint16_t>(packet[0]) << 8) | packet[1];
	REQUIRE(schemaVersion == 1);
	uint8_t targetCount = packet[2];
	REQUIRE(targetCount == ids.size());

	std::string status = clientTable->GetString(".status", "");
	nlohmann::json statusJson = nlohmann::json::parse(status, nullptr, false /* allow_exceptions */);
	REQUIRE_FALSE(statusJson.is_discarded());
	REQUIRE(statusJson.contains("uptimeSeconds"));

	client.StopClient();
	server.StopServer();
}

TEST_CASE("NetworkTablesSink surfaces robot-writable config/pipelineIndex and config/driverMode writes", "[nt4]") {
	constexpr unsigned int TEST_NT3_PORT = 17811;
	constexpr unsigned int TEST_NT4_PORT = 17812;

	nt::NetworkTableInstance server = nt::NetworkTableInstance::Create();
	server.StartServer("", "127.0.0.1", TEST_NT3_PORT, TEST_NT4_PORT);

	auto logger = std::make_shared<Logger>("LumenCoreTests-nt4-config.log");
	NetworkTablesConfig config;
	config.serverAddress = "127.0.0.1";
	config.port = TEST_NT4_PORT;
	config.rootTable = "lumenvision";
	config.clientIdentity = "LumenCoreTests-nt4-config-sink";
	auto ntSink = std::make_shared<NetworkTablesSink>(logger, "nt4-config-sink", config);

	// nothing bound, and no Process() tick ever runs - PollConfigRequests must work purely off
	// the listener callback, independent of this sink's own publish cadence.
	REQUIRE(ntSink->PollConfigRequests() == "[]");

	nt::NetworkTableInstance robot = nt::NetworkTableInstance::Create();
	robot.SetServer("127.0.0.1", TEST_NT4_PORT);
	robot.StartClient4("LumenCoreTests-nt4-config-robot");
	auto robotSourceTable = robot.GetTable("lumenvision/some-detector-id");
	robotSourceTable->PutBoolean("config/driverMode", true);
	robotSourceTable->PutNumber("config/pipelineIndex", 3);

	nlohmann::json requests;
	REQUIRE(WaitUntil([&] {
		requests = nlohmann::json::parse(ntSink->PollConfigRequests(), nullptr, false);
		return !requests.is_discarded() && !requests.empty();
	}));
	REQUIRE(requests.size() == 1);
	REQUIRE(requests[0]["sourceId"] == "some-detector-id");
	REQUIRE(requests[0]["driverMode"] == true);
	REQUIRE(requests[0]["pipelineIndex"] == 3);

	// consumed, not just read - a second poll with no new writes must come back empty
	REQUIRE(ntSink->PollConfigRequests() == "[]");

	robot.StopClient();
	server.StopServer();
}

TEST_CASE("NetworkTablesSink surfaces the robot's config/recording request and publishes status/recording", "[nt4]") {
	constexpr unsigned int TEST_NT3_PORT = 17821;
	constexpr unsigned int TEST_NT4_PORT = 17822;

	nt::NetworkTableInstance server = nt::NetworkTableInstance::Create();
	server.StartServer("", "127.0.0.1", TEST_NT3_PORT, TEST_NT4_PORT);

	auto logger = std::make_shared<Logger>("LumenCoreTests-nt4-recording.log");
	NetworkTablesConfig config;
	config.serverAddress = "127.0.0.1";
	config.port = TEST_NT4_PORT;
	config.rootTable = "lumenvision";
	config.clientIdentity = "LumenCoreTests-nt4-recording-sink";
	auto ntSink = std::make_shared<NetworkTablesSink>(logger, "nt4-recording-sink", config);

	REQUIRE(ntSink->PollRecordingRequest() == -1); // nothing written yet

	nt::NetworkTableInstance robot = nt::NetworkTableInstance::Create();
	robot.SetServer("127.0.0.1", TEST_NT4_PORT);
	robot.StartClient4("LumenCoreTests-nt4-recording-robot");
	// exactly what photoncompat's LumenCoprocessor.setRecording() publishes
	auto recordingPub = robot.GetBooleanTopic("/lumenvision/config/recording").Publish();
	recordingPub.Set(true);

	int request = -1;
	REQUIRE(WaitUntil([&] { request = ntSink->PollRecordingRequest(); return request != -1; }));
	REQUIRE(request == 1);
	REQUIRE(ntSink->PollRecordingRequest() == -1); // consumed
	// a coprocessor-wide topic must not leak into the per-source config array
	REQUIRE(ntSink->PollConfigRequests() == "[]");

	recordingPub.Set(false);
	REQUIRE(WaitUntil([&] { request = ntSink->PollRecordingRequest(); return request != -1; }));
	REQUIRE(request == 0);

	// status flows the other way: what LumenCoprocessor.isRecording() reads
	auto statusSub = robot.GetBooleanTopic("/lumenvision/status/recording").Subscribe(false);
	ntSink->SetRecordingStatus(true);
	REQUIRE(WaitUntil([&] { return statusSub.Get() == true; }));
	ntSink->SetRecordingStatus(false);
	REQUIRE(WaitUntil([&] { return statusSub.Get() == false; }));

	robot.StopClient();
	server.StopServer();
}
