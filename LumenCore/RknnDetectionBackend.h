#pragma once
#ifdef LUMEN_WITH_RKNN

#include "IDetectionBackend.h"
#include <rknn_api.h>
#include <string>
#include <vector>

// Runs a YOLOv8/v11 export on the RK3588's own NPU via the RKNN runtime (librknnrt.so) -
// ROADMAP.md Phase 6's headline win: roughly 100ms/frame on the A76 CPU cluster (ONNX Runtime)
// down to 10-20ms on one NPU core, verified against PhotonVision's own shipped
// yolov8nCOCO.rknn/fuelV1-yolo11n.rknn models on the bench Orange Pi.
//
// Deliberately reuses YoloPostProcess::DecodeAndNms (the exact same decoder OnnxDetectionBackend
// uses) rather than a separate RKNN-specific decode path - both backends produce the same
// anchor-free [4+numClasses, numAnchors] head layout in the end, and rknn_output's own
// want_float=1 flag does the int8/fp16 dequantization for us, so there is no format difference
// left for this code to handle by the time DecodeAndNms sees it.
class RknnDetectionBackend : public IDetectionBackend {
public:
	RknnDetectionBackend();
	~RknnDetectionBackend() override;

	bool Load(const DetectionBackendConfig& config) override;
	std::vector<ObjectDetection> Infer(const cv::Mat& bgrFrame) override;
	std::string Name() const override { return "RKNN (NPU)"; }

private:
	// one FPN scale of an un-fused, multi-output DFL export (the airockchip/rknn_model_zoo
	// recipe PhotonVision's own shipped models use - confirmed against the real
	// yolov8nCOCO.rknn/fuelV1-yolo11n.rknn on the bench Orange Pi: 9 outputs, grouped in triples
	// of (box[4*regMax,H,W], class[numClasses,H,W], scoreSum[1,H,W] - unused, a fast NPU-side
	// pre-filter this CPU-side decode doesn't need)). See YoloPostProcess::DecodeDflMultiScaleAndNms.
	struct ScaleMeta {
		uint32_t boxOutputIndex, clsOutputIndex;
		int gridH, gridW, stride, regMax;
	};

	rknn_context m_Context = 0;
	bool m_Loaded = false;
	bool m_IsMultiScaleDfl = false;

	DetectionBackendConfig m_Config;
	std::vector<std::string> m_Labels;

	rknn_tensor_attr m_InputAttr{};
	int m_InputWidth = 0, m_InputHeight = 0;
	int m_NumOutputs = 0;
	int m_NumClasses = 0;

	// n_output==1 case (a fused single-head export, matching ONNX's own layout) - kept as a
	// fallback for any RKNN model NOT exported via the rknn_model_zoo multi-output recipe.
	rknn_tensor_attr m_FusedOutputAttr{};

	std::vector<ScaleMeta> m_Scales;
};

#endif // LUMEN_WITH_RKNN
