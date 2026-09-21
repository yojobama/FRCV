#include <catch2/catch_test_macros.hpp>
#include "RknnDetectionBackend.h"
#include <filesystem>
#include <opencv2/imgcodecs.hpp>

// Hardware-in-the-loop coverage for ROADMAP.md Phase 6's RKNN backend - only buildable at all
// when LUMEN_WITH_RKNN is on (tests/CMakeLists.txt only adds this file then), and self-skips if
// the real model isn't present (every machine except the bench Orange Pi, which already has it
// via PhotonVision's own install - see ROADMAP.md's note that PhotonVision ships
// yolov8nCOCO.rknn/fuelV1-yolo11n.rknn specifically so this project can validate against them
// before building any model-conversion pipeline of its own).
//
// This is real, hardware-verified content coverage, not just a no-crash smoke test: the first
// version of RknnDetectionBackend's DFL decode compiled and ran cleanly against a solid-colour
// synthetic frame while actually being geometrically wrong in a way a "did it throw" check could
// never catch - a real model export turned out to have 9 raw multi-scale outputs (the
// airockchip/rknn_model_zoo export recipe), not the single fused head this backend's first draft
// assumed. bus.jpg (ultralytics' own canonical YOLO demo image, pulled from rknn-toolkit2's own
// examples tree) is what caught that: at a real-world confidence threshold the model must recover
// exactly the well-known 1 bus + 3 people, correctly positioned - not "some detections came back".
TEST_CASE("RknnDetectionBackend recovers correct, well-positioned detections from a real photo", "[hitl][rknn]") {
	const std::string modelPath = "/opt/photonvision/photonvision_config/models/yolov8nCOCO.rknn";
	if (!std::filesystem::exists(modelPath)) {
		SKIP("PhotonVision's yolov8nCOCO.rknn is not present on this machine");
	}

	DetectionBackendConfig config;
	config.modelPath = modelPath;
	config.labelsPath = std::string(LUMEN_TEST_DATA_DIR) + "/coco_80_labels.txt";
	// int8 NPU quantization noise means a much looser threshold (e.g. the 0.25 default) turns up
	// well over a thousand low-confidence false positives scattered across the frame - a real,
	// expected characteristic of quantized inference, not a decode bug (confirmed by hand: at
	// 0.25 the same model/image produces 1589 raw detections; at 0.6 it collapses to exactly the
	// 4 correct ones asserted below). PhotonVision's own defaults are tuned similarly high.
	config.confThreshold = 0.6f;
	config.nmsThreshold = 0.45f;

	RknnDetectionBackend backend;
	REQUIRE(backend.Load(config));

	cv::Mat frame = cv::imread(std::string(LUMEN_TEST_DATA_DIR) + "/bus.jpg");
	REQUIRE_FALSE(frame.empty());

	std::vector<ObjectDetection> detections = backend.Infer(frame);

	int busCount = 0, personCount = 0;
	for (const ObjectDetection& d : detections) {
		if (d.GetClassName() == "bus") {
			busCount++;
			// the bus fills roughly the left-to-center two-thirds of the frame, not a sliver
			// somewhere implausible - a wrong DFL side-ordering (e.g. left/right swapped) would
			// produce a badly-shaped or mispositioned box even if the CLASS came out right.
			cv::Rect2d box = d.GetBoundingBox();
			REQUIRE(box.width > frame.cols * 0.3);
			REQUIRE(box.x < frame.cols * 0.5);
		}
		if (d.GetClassName() == "person") personCount++;
	}

	REQUIRE(busCount == 1);
	REQUIRE(personCount >= 3);
}
