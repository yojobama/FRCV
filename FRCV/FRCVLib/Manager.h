#pragma once
#include <vector>
#include <string>
#include <map>
#include <random>
#include <chrono>
#include <pthread.h>
#include <memory>

#include "ISink.h"
#include "CameraCalibrationResult.h"
#include "Logger.h"
#include "ISource.h"

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
	int CreateObjectDetectionSink(ObjectDetectionProvider provider);
	int CreateObjectDetectionSink(ObjectDetectionProvider provider, int id);

	// functions to create/manage camera calibrators
	int CreateCameraCalibrator();
	int CreateCameraCalibrator(int id);

	// retrieves the calibration result of a CameraCalibrator sink, identified via dynamic_cast
	CameraCalibrationResult GetCameraCalibrationResult(int calibratorId);

	// saves the checkerboard corners detected in the CameraCalibrator's latest frame as a calibration
	// snapshot to be used in the calibration phase. returns false if no board was detected yet.
	bool SaveCameraCalibrationBoardDetection(int calibratorId);

	// creates an ApriltagDetector using the calibration result produced by an existing CameraCalibrator,
	// transferring the calibration data so the detector can compute the real-world tag location
	int CreateApriltagDetectorFromCalibrator(int calibratorId, double tagSize /* in METERS you filthy Americans! */);
	int CreateApriltagDetectorFromCalibrator(int id, int calibratorId, double tagSize /* in METERS you filthy Americans! */);

	int CreateRecordingSink(int sourceId);

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

