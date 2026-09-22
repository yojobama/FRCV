#pragma once
#include <vector>
#include <string>
#include <map>
#include <random>
#include <chrono>
#include <memory>

#include "ISink.h"
#include "CameraCalibrationResult.h"
#include "Logger.h"
#include "ISource.h"
#include "IDetectionBackend.h"
#include "IApriltagBackend.h"
#include "CalibrationBoardType.h"
#include "StereoCalibrationResult.h"
#include "StereoDepthBackendKind.h"
#include "StereoFrameOutput.h"
#include "CameraMode.h"

using namespace std;

// TODO: temporary, rewrite this part
typedef struct {
	string name;
	string path;
} CameraHardwareInfo;

enum ObjectDetectionProvider
{
	RKNN,
	ONNX
};

//enum Platform {
//	ORANGE_PI,
//	JETSON,
//	NVDIA_DEV_X86_64,
//	CPU_DEV_X86_64
//};

//class CameraCalibrationSink;
class SystemMonitor;

class Manager
{
public:
	Manager(string logFile);
	Manager();
	~Manager();

	// utill functions
	vector<int> GetAllSinks();
	vector<int> GetAllSources();

	std::vector<std::string> GetAvailableVideoEncoders();

	vector<CameraHardwareInfo> EnumerateAvailableCameras();
	bool BindSourceToSink(int sourceId, int sinkId);
	bool UnbindSourceFromSink(int sinkId);

	// functions to create frame sources
	int CreateCameraSource(CameraHardwareInfo info);
	int CreateCameraSource(CameraHardwareInfo info, int id);

	// Explicit resolution/fps/exposure control (ROADMAP.md Phase 3b) - throws if sourceId isn't a
	// camera source at all, since that's a caller bug, not a routine failure the way an
	// unsupported hardware control (a false return) is.
	vector<CameraMode> GetCameraModes(int sourceId);
	CameraMode GetCameraCurrentMode(int sourceId);
	bool SetCameraMode(int sourceId, CameraMode mode);
	bool SetCameraExposure(int sourceId, int exposureAbsolute);
	bool SetCameraAutoExposure(int sourceId, bool enabled);
	bool SetCameraGain(int sourceId, int gain);

	// splits one upstream source's frames into a fixed rectangular crop, zero-copy (ROADMAP.md
	// Phase 3d) - the building block for side-by-side/top-bottom stereo: create two of these
	// against the same upstream camera (one per eye's half), then bind each one into
	// StereoCalibrator/StereoDepthNode exactly like two independent cameras. Binds itself to
	// upstreamSourceId automatically; throws if upstreamSourceId doesn't exist.
	int CreateRoiSource(int upstreamSourceId, int x, int y, int width, int height);

	// ROADMAP.md Phase 7 (driver mode) - throws if sinkId isn't a detection sink that actually
	// supports it (ApriltagDetector/ObjectDetectionSink today), since that's a caller bug, not a
	// routine failure.
	void SetDriverMode(int sinkId, bool enabled);
	bool GetDriverMode(int sinkId);

	// ROADMAP.md Phase 7 (multi-tag PnP) - loads a WPILib-format AprilTagFieldLayout JSON file
	// onto an ApriltagDetector sink; throws if sinkId isn't one. Once loaded, every frame with
	// 2+ simultaneously-visible tags that have known field poses gets a single, jointly-solved
	// field-relative camera pose published alongside the existing per-tag detections.
	bool LoadFieldLayout(int sinkId, string jsonPath);
	int GetFieldLayoutTagCount(int sinkId);

	// ROADMAP.md Phase 7 (snapshots) - saves sourceId's most recently published frame to a file
	// (format inferred from the extension, via cv::imwrite - .png/.jpg/etc). Returns false if
	// sourceId has never published a frame yet or the write itself fails (a bad path, an
	// unwritable directory); throws only if sourceId doesn't exist at all.
	bool SaveSnapshot(int sourceId, string path);
	int CreateVideoFileSource(string path, int fps);
	int CreateVideoFileSource(string path, int fps, int id);
	int CreateImageFileSource(string path);
	int CreateImageFileSource(string path, int id);

	// functions to create detection sinks
	int CreateApriltagDetector(CameraCalibrationResult calibrationResult, double tagSize /* in METERS you filthy Americans! */);
	int CreateApriltagDetector(int id, CameraCalibrationResult calibrationResult, double tagSize /* in METERS you filthy Americans! */);
	// creates an ApriltagDetector with an empty calibration result; use BindSourceToSink + CreateApriltagDetectorFromCalibrator
	// (or GetCameraCalibrationResult) to supply real calibration data once available
	int CreateApriltagDetector();
	int CreateApriltagDetector(int id);

	// same as the overloads above, but with an explicit backend selection. frameWidth/
	// frameHeight are only consulted for APRILTAG_BACKEND_VULKAN (the GPU pipeline's buffers
	// are sized once at construction, unlike the CPU backend which is frame-size-agnostic); if
	// Vulkan is requested but unavailable, the node falls back to CPU rather than failing to
	// construct (check GetApriltagDetectorBackendName afterwards to see which one actually ran).
	// nthreads/quadDecimate <=0 means "let the backend pick its own default" - see
	// ApriltagDetector's own constructor comment.
	int CreateApriltagDetector(CameraCalibrationResult calibrationResult, double tagSize,
		ApriltagBackendKind backendKind, int frameWidth, int frameHeight,
		int nthreads = 0, float quadDecimate = 0.0f);
	int CreateApriltagDetector(int id, CameraCalibrationResult calibrationResult, double tagSize,
		ApriltagBackendKind backendKind, int frameWidth, int frameHeight,
		int nthreads = 0, float quadDecimate = 0.0f);
	string GetApriltagDetectorBackendName(int sinkId);
	// which backend was actually requested/resolved to (as a real enum, not just the display
	// name GetApriltagDetectorBackendName returns) plus everything else needed to rebuild an
	// equivalent detector - the webui's Inspector "Backend"/"Threads"/"Quad Decimate" controls
	// on a plain-created ApriltagSink (one not made via a Pipeline Profile) need all of these to
	// recreate the sink without silently dropping its tag size/calibration/tuning.
	ApriltagBackendKind GetApriltagDetectorBackendKind(int sinkId);
	double GetApriltagDetectorTagSize(int sinkId);
	CameraCalibrationResult GetApriltagDetectorCalibration(int sinkId);
	int GetApriltagDetectorThreads(int sinkId);
	float GetApriltagDetectorQuadDecimate(int sinkId);
	bool GetApriltagDetectorQuadDecimateSupported(int sinkId);
	// legacy no-model overloads: there is no way to run inference without a model, so these
	// exist only to keep already-generated SWIG call sites compiling and throw a clear error
	// explaining that a model must be supplied via the overload below
	int CreateObjectDetectionSink(ObjectDetectionProvider provider);
	int CreateObjectDetectionSink(ObjectDetectionProvider provider, int id);

	// RKNN runs on the RK3588's own NPU (see RknnDetectionBackend's own comment) - only compiled
	// in when LUMEN_WITH_RKNN is on (the Pi build; not WSL/Windows, which have no RK3588 NPU to
	// target), in which case selecting RKNN without that build flag throws a clear error rather
	// than silently falling back to ONNX.
	int CreateObjectDetectionSink(ObjectDetectionProvider provider, string modelPath, string labelsPath,
		YoloVariant variant, float confThreshold, float nmsThreshold, int inputSize);
	int CreateObjectDetectionSink(int id, ObjectDetectionProvider provider, string modelPath, string labelsPath,
		YoloVariant variant, float confThreshold, float nmsThreshold, int inputSize);

	// which backend an existing ObjectDetectionSink actually ended up running - see
	// ObjectDetectionSink::GetBackendName's own comment on why this is read-only/informational
	// rather than a live setter the way ApriltagDetector's backend choice is.
	std::string GetObjectDetectionSinkBackendName(int sinkId);

	// functions to create/manage camera calibrators. Default board is a 6x9 checkerboard with
	// 25mm squares, matching the previous hardcoded behavior; the explicit-config overloads
	// take only primitive parameters (plus the plain CalibrationBoardType enum) rather than
	// CalibrationBoardConfig itself - CameraCalibrator.h pulls in
	// <opencv2/objdetect/charuco_detector.hpp>, a much heavier header than SWIG has been fed
	// so far in this project and untested against it, so it must not reach swig.i.
	int CreateCameraCalibrator();
	int CreateCameraCalibrator(int id);
	int CreateCameraCalibrator(CalibrationBoardType boardType, int rows, int cols,
		float squareSizeMeters, float markerSizeMeters, int arucoDictionaryId);
	int CreateCameraCalibrator(int id, CalibrationBoardType boardType, int rows, int cols,
		float squareSizeMeters, float markerSizeMeters, int arucoDictionaryId);

	// retrieves the calibration result of a CameraCalibrator sink, identified via dynamic_cast
	CameraCalibrationResult GetCameraCalibrationResult(int calibratorId);
	// explicitly runs cv::calibrateCamera over every snapshot saved so far and caches the
	// result (also returned by GetCameraCalibrationResult afterwards); throws if fewer than 4
	// snapshots have been saved
	CameraCalibrationResult RunCameraCalibration(int calibratorId);

	int GetCameraCalibrationSnapshotCount(int calibratorId);
	bool RemoveCameraCalibrationSnapshot(int calibratorId, int index);
	void ClearCameraCalibrationSnapshots(int calibratorId);

	// ROADMAP.md Phase 8a/8d: coverage-heatmap data for the calibration wizard - one saved
	// snapshot's corner points, flattened [x0,y0,x1,y1,...]; empty if calibratorId doesn't exist
	// or index is out of range. GetCameraCalibrationFrameWidth/Height are 0 until at least one
	// snapshot has been saved.
	vector<double> GetCameraCalibrationSnapshotCorners(int calibratorId, int index);
	int GetCameraCalibrationFrameWidth(int calibratorId);
	int GetCameraCalibrationFrameHeight(int calibratorId);

	// saves the checkerboard corners detected in the CameraCalibrator's latest frame as a calibration
	// snapshot to be used in the calibration phase. returns false if no board was detected yet.
	bool SaveCameraCalibrationBoardDetection(int calibratorId);

	// creates an ApriltagDetector using the calibration result produced by an existing CameraCalibrator,
	// transferring the calibration data so the detector can compute the real-world tag location
	int CreateApriltagDetectorFromCalibrator(int calibratorId, double tagSize /* in METERS you filthy Americans! */);
	int CreateApriltagDetectorFromCalibrator(int id, int calibratorId, double tagSize /* in METERS you filthy Americans! */);

	// --- Stereo depth (phase 10) - see STEREO_IMPLEMENTATION_PLAN.md ---
	//
	// StereoCalibrator: same "primitives only" discipline as CameraCalibrator above -
	// StereoCalibrator.h pulls in <opencv2/calib3d.hpp> and must not reach swig.i. ChArUco is
	// not supported (see StereoCalibrator.h), so unlike CreateCameraCalibrator there is no
	// marker size / dictionary parameter here.
	int CreateStereoCalibrator();
	int CreateStereoCalibrator(int id);
	int CreateStereoCalibrator(CalibrationBoardType boardType, int rows, int cols, float squareSizeMeters);
	int CreateStereoCalibrator(int id, CalibrationBoardType boardType, int rows, int cols, float squareSizeMeters);

	// binds two sources as the explicit left/right roles of a stereo sink (StereoCalibrator or
	// StereoDepthNode) - ordinary BindSourceToSink is bind-order only and getting left/right
	// backwards silently flips the sign of every disparity, so this both binds (via the normal
	// ISink::BindSource path) AND records the roles explicitly by source ID. See
	// IStereoRoleReceiver.h / STEREO_IMPLEMENTATION_PLAN.md P2.
	bool BindStereoSources(int sinkId, int leftSourceId, int rightSourceId);

	bool SaveStereoCalibrationDetection(int calibratorId);
	int GetStereoCalibrationPairCount(int calibratorId);
	bool RemoveStereoCalibrationPair(int calibratorId, int index);
	void ClearStereoCalibrationPairs(int calibratorId);

	// same reasoning as GetCameraCalibrationSnapshotCorners - eye must be "left" or "right".
	vector<double> GetStereoCalibrationPairCorners(int calibratorId, int index, string eye);
	int GetStereoCalibrationFrameWidth(int calibratorId);
	int GetStereoCalibrationFrameHeight(int calibratorId);
	// runs cv::stereoCalibrate + cv::stereoRectify over every saved pair; throws if fewer than 8
	// pairs have been saved. Also returned by GetStereoCalibrationResult afterwards.
	StereoCalibrationResult RunStereoCalibration(int calibratorId);
	StereoCalibrationResult GetStereoCalibrationResult(int calibratorId);

	// StereoDepthNode: bind with BindStereoSources, same as StereoCalibrator. `calibration` is
	// normally the result of RunStereoCalibration on a StereoCalibrator sink - fetch it via
	// GetStereoCalibrationResult and pass it straight through.
	int CreateStereoDepthNode(StereoDepthBackendKind backend, StereoCalibrationResult calibration,
		double minDepthMeters, double maxDepthMeters, int maxSkewUs, StereoFrameOutput frameOutput);
	int CreateStereoDepthNode(int id, StereoDepthBackendKind backend, StereoCalibrationResult calibration,
		double minDepthMeters, double maxDepthMeters, int maxSkewUs, StereoFrameOutput frameOutput);
	// which backend actually ended up running (e.g. "lavc_sw", "rkmpp_hwenc", "sgbm")
	string GetStereoDepthBackendName(int sinkId);
	// fraction of blocks that came back valid in the most recently processed pair, and their
	// median depth in meters - the summary stats a WebUI/NT4 caller actually wants, not the
	// full per-block grid (see GetSinkResult's JSON for the rest - STEREO_IMPLEMENTATION_PLAN.md
	// ss10.3 "Outputs").
	double GetStereoDepthValidFraction(int sinkId);
	double GetStereoDepthMedianDepthMeters(int sinkId);

	// DepthFusionNode: fuses a detector's bounding boxes with a StereoDepthNode's depth grid -
	// see STEREO_IMPLEMENTATION_PLAN.md ss10.4. Bind the detector with the ordinary
	// BindSourceToSink (it must itself be bound to the StereoDepthNode's own rectified-left
	// frame output, not a raw camera - see DepthFusionNode.h); attach the depth source
	// separately here, since it's read directly in-process rather than through the normal
	// ISource/ISink result path (the full depth grid is deliberately never serialized through
	// SourceResult/JSON - see StereoDepthNode::GetLastDepthGrid's own comment).
	int CreateDepthFusionNode();
	int CreateDepthFusionNode(int id);
	bool SetDepthFusionDepthNode(int fusionSinkId, int depthNodeSourceId);

	// terminal sink: bind any JSON-producing source (ApriltagDetector, CameraCalibrator, future
	// ObjectDetectionSink) to it and it publishes onto the configured NT4 server. Deliberately
	// takes only primitive parameters rather than a config struct straight from
	// NetworkTablesSink.h — that header pulls in ntcore's C++ API, which SWIG (parsing this file
	// for the C# bindings) is not expected to handle, so it must never appear in this header.
	//
	// Declared unconditionally (not #ifdef LUMEN_WITH_NT4) even though the implementation is:
	// a build with NT4 compiled out must still export these symbols so every configuration
	// generates the SAME C# API from swig.i (see cmake/LumenFeatures.cmake's LUMEN_SWIG_DEFINES
	// vs LUMEN_ENABLED_DEFINES split) - the alternative is two builds silently exposing
	// different methods, which is worse than one build where calling a disabled one throws a
	// clear "not compiled into this build" error. See Manager.cpp / GetEnabledFeatures().
	int CreateNetworkTablesSinkForTeam(int teamNumber, string rootTable, string clientIdentity);
	int CreateNetworkTablesSinkForTeam(int id, int teamNumber, string rootTable, string clientIdentity);
	int CreateNetworkTablesSinkForServer(string serverAddress, int port, string rootTable, string clientIdentity);
	int CreateNetworkTablesSinkForServer(int id, string serverAddress, int port, string rootTable, string clientIdentity);
	bool IsNetworkTablesSinkConnected(int sinkId);
	string GetNetworkTablesSinkStatus(int sinkId);

	// terminal sink: bind any single frame-producing node (raw camera, or a detector's
	// annotated output) and it encodes+streams it over WebRTC. Deliberately takes only
	// primitive parameters - WebRTCSink.h pulls in libdatachannel's C++ API, which (like
	// NetworkTablesSink's ntcore) must never reach swig.i. Declared unconditionally for the
	// same reason as the NT4 methods above.
	int CreateWebRTCSink(int bitrateKbps, int fps, string encoderName);
	int CreateWebRTCSink(int id, int bitrateKbps, int fps, string encoderName);
	// "h264_rkmpp" if this build's ffmpeg actually has it (real RK3588 hardware encode via
	// nyanmisaka/ffmpeg-rockchip - see ROADMAP.md Phase 6 and cmake/LumenFFmpeg.cmake's own
	// comment on why upstream FFmpeg's --enable-rkmpp is decode-only), else "libx264" - a real
	// runtime probe (avcodec_find_encoder_by_name), not a platform guess, so this stays correct
	// even on a Linux/aarch64 build that happens not to have the hardware ffmpeg build installed.
	string GetPreferredWebRTCEncoder();
	// non-trickle ICE: blocks until this peer's candidate gathering completes (bounded by a
	// timeout inside WebRTCSink), then returns one complete SDP offer
	string WebRTCCreateOffer(int sinkId);
	void WebRTCSetAnswer(int sinkId, string sdp);
	void WebRTCAddIceCandidate(int sinkId, string candidate, string mid);
	bool IsWebRTCSinkConnected(int sinkId);
	string GetWebRTCSinkStatus(int sinkId);

	// which LUMEN_WITH_* backends this build actually has compiled in (e.g. {"ONNX", "NT4",
	// "VULKAN_APRILTAG"}) - lets the WebUI grey out unavailable options instead of discovering
	// them by a failed request, and is the runtime counterpart to the SWIG-surface-stays-constant
	// design above: the API always exists, this is how a caller finds out what actually works.
	vector<string> GetEnabledFeatures();

	// stops and removes a node; also unbinds it from any sink that referenced it as a source
	bool DeleteSink(int sinkId);
	bool DeleteSource(int sourceId);

	void StartAllSources();
	void StopAllSources();
	bool StopSourceById(int sourceId);
	bool StartSourceById(int sourceId);
	bool IsSourceActive(int sourceId);

	void StartAllSinks();
	void StopAllSinks();
	bool StartSinkById(int sinkId);
	bool StopSinkById(int sinkId);
	bool IsSinkActive(int sinkId);

	string GetSinkResult(int sinkId);
	string GetAllSinkResults();

	//vector<string> GetRecording(int recorderId); // TODO: implement a recording mechanisem

	//int CreateCameraCalibrationSink(int width, int height);
	//void BindSourceToCalibrationSink(int sourceId);
	//void CameraCalibrationSinkGrabFrame(int sinkId);

	//CameraCalibrationResult GetCameraCalibrationResults(int sinkId);
	
	// functions to check system status
	int GetMemoryUsageBytes();
	int GetCPUUsage();
	int GetCpuTemperature();
	int GetDiskUsage();

	// ROADMAP.md Phase 8a: live per-node throughput for the /ws/state channel and Phase 8e's
	// match view. Works for any id currently in m_Sources - a real camera/file source, or a
	// dual-role sink acting as its own source (ApriltagDetector, ObjectDetectionSink, etc. - see
	// the Manager::DeleteSink comment on why these share one id space). 0 for anything else
	// (terminal sinks like NetworkTablesSink/WebRTCSink have no frame count of their own to
	// report - they consume, not produce).
	//
	// FPS itself is deliberately NOT computed here: it needs two samples over a known time
	// delta, which is exactly what the C# WS broadcast loop already does once per tick for every
	// tracked node - duplicating that bookkeeping natively would just be a second copy to keep in
	// sync. GetFrameCount is the raw counter; the delta is taken where it's consumed.
	uint64_t GetFrameCount(int id);
	// producedTimeUs - captureTimeUs of the most recent frame; 0 if id doesn't exist in
	// m_Sources or no frame has been produced yet.
	int64_t GetLatencyUs(int id);
private:
	//bool SetSinkResult(int sinkId, string result);

	int GenerateUUID();

	// maps for storing results, sources and sinks
	map<int, std::shared_ptr<ISource>> m_Sources;
	map<int, std::shared_ptr<ISink>> m_Sinks;
	//map<int, std::shared_ptr<CameraCalibrationSink>> m_CameraCalibrationSinks; // camera calibration sinks

	std::shared_ptr<Logger> m_Logger; // a logger for the entire application

	SystemMonitor* m_SystemMonitor; // system monitor for CPU, memory and disk usage
};

