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
#include "FramePool.h"
#include "ISource.h"

using namespace std;

// TODO: temporary, rewrite this part
typedef struct {
	string name;
	string path;
} CameraHardwareInfo;

//enum Platform {
//	ORANGE_PI,
//	JETSON,
//	NVDIA_DEV_X86_64,
//	CPU_DEV_X86_64
//};

class PreProcessor;
class Frame;
class CameraCalibrationSink;
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
	int CreateApriltagSink();
	int CreateApriltagSink(int id);
	int CreateObjectDetectionSink();
	int CreateObjectDetectionSink(int id);

	int CreateRecordingSink(int sourceId);

	void StartAllSources();
	void StopAllSources();
	bool StopSourceById(int sourceId);
	bool StartSourceById(int sourceId);

	void StartAllSinks();
	void StopAllSinks();
	bool StartSinkById(int sinkId);
	bool StopSinkById(int sinkId);

	string GetAllSinkStatus();

	string GetSinkStatusById(int sinkId);

	string GetSinkResult(int sinkId);
	string GetAllSinkResults();

	//vector<string> GetRecording(int recorderId); // TODO: implement a recording mechanisem

	int CreateCameraCalibrationSink(int height, int width);
	void BindSourceToCalibrationSink(int sourceId);
	void CameraCalibrationSinkGrabFrame(int sinkId);

	CameraCalibrationResult GetCameraCalibrationResults(int sinkId);
	
	// functions to check system status
	int GetMemoryUsageBytes();
	int GetCPUUsage();
	int GetCpuTemperature();
	int GetDiskUsage();

private:
	bool SetSinkResult(int sinkId, string result);

	int GenerateUUID();

	// maps for storing results, sources and sinks
	map<int, ISource*> m_Sources;
	map<int, ISink*> m_Sinks;
	map<int, CameraCalibrationSink*> m_CameraCalibrationSinks; // camera calibration sinks

	FramePool* m_FramePool;

	PreProcessor* m_PreProcessor;

	Logger* m_Logger; // a logger for the entire application

	vector<std::shared_ptr<Frame>> m_CalibrationImages;

	SystemMonitor* m_SystemMonitor; // system monitor for CPU, memory and disk usage
};

