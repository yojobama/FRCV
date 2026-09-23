#include <catch2/catch_test_macros.hpp>
#include "OnnxDetectionBackend.h"
#include <opencv2/imgcodecs.hpp>

// Golden-corpus regression coverage for OnnxDetectionBackend (ROADMAP.md Phase D) - the one
// object-detection golden test that's genuinely local: ONNX Runtime's CPU execution provider
// needs no RK3588 NPU, so this runs everywhere, no [hitl] label. Reuses the exact same checked-in
// fixture (bus.jpg, coco_80_labels.txt) and expected-detection shape (1 bus spanning roughly the
// left two-thirds of the frame, at least 3 people) as test_rknn_detection_hitl.cpp - same model
// family (YOLOv8n/COCO), same image, so the two backends' results are directly comparable even
// though this test doesn't require the RKNN one to actually run on this machine.
TEST_CASE("OnnxDetectionBackend recovers correct, well-positioned detections from a real photo", "[onnx]") {
	DetectionBackendConfig config;
	config.modelPath = std::string(LUMEN_TEST_DATA_DIR) + "/yolov8n.onnx";
	config.labelsPath = std::string(LUMEN_TEST_DATA_DIR) + "/coco_80_labels.txt";
	config.variant = YOLOv8;
	config.confThreshold = 0.5f;
	config.nmsThreshold = 0.45f;
	config.inputWidth = 640;
	config.inputHeight = 640;

	OnnxDetectionBackend backend;
	REQUIRE(backend.Load(config));

	cv::Mat frame = cv::imread(std::string(LUMEN_TEST_DATA_DIR) + "/bus.jpg");
	REQUIRE_FALSE(frame.empty());

	std::vector<ObjectDetection> detections = backend.Infer(frame);

	int busCount = 0, personCount = 0;
	for (const ObjectDetection& d : detections) {
		if (d.GetClassName() == "bus") {
			busCount++;
			// same positional sanity check as the RKNN test - a wrong box-decode ordering
			// produces a badly-shaped or mispositioned box even if the CLASS came out right.
			cv::Rect2d box = d.GetBoundingBox();
			REQUIRE(box.width > frame.cols * 0.3);
			REQUIRE(box.x < frame.cols * 0.5);
		}
		if (d.GetClassName() == "person") personCount++;
	}

	REQUIRE(busCount == 1);
	REQUIRE(personCount >= 3);
}
