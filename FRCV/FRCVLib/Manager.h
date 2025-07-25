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

class Manager
{
public:
	Manager(string logFile);
	Manager();
	~Manager();

	// utill functions
	vector<int> getAllSinks();
	vector<int> getAllSources();

	vector<CameraHardwareInfo> enumerateAvailableCameras();
	bool bindSourceToSink(int sourceId, int sinkId);
	bool unbindSourceFromSink(int sinkId);

	// functions to create frame sources
	int createCameraSource(CameraHardwareInfo info);
	int createCameraSource(CameraHardwareInfo info, int id);
	int createVideoFileSource(string path, int fps);
	int createVideoFileSource(string path, int fps, int id);
	int createImageFileSource(string path);
	int createImageFileSource(string path, int id);

	// functions to create detection sinks
	int createApriltagSink();
	int createApriltagSink(int id);
	int createObjectDetectionSink();
	int createObjectDetectionSink(int id);

	int createRecordingSink(int sourceId);

	void startAllSources();
	void stopAllSources();
	bool stopSourceById(int sourceId);
	bool startSourceById(int sourceId);

	void startAllSinks();
	void stopAllSinks();
	bool startSinkById(int sinkId);
	bool stopSinkById(int sinkId);

	string getAllSinkStatus();

	string getSinkStatusById(int sinkId);

	string getSinkResult(int sinkId);
	string getAllSinkResults();

	//vector<string> getRecording(int recorderId); // TODO: implement a recording mechanisem

	int createCameraCalibrationSink(int height, int width);
	void bindSourceToCalibrationSink(int sourceId);
	void cameraCalibrationSinkGrabFrame(int sinkId);

	CameraCalibrationResult getCameraCalibrationResults(int sinkId);
private:
	bool setSinkResult(int sinkId, string result);

	int generateUUID();

	// maps for storing results, sources and sinks
	map<int, ISource*> sources;
	map<int, ISink*> sinks;
	map<int, CameraCalibrationSink*> cameraCalibrationSinks; // camera calibration sinks

	FramePool *framePool;

	PreProcessor* preProcessor;

	Logger* logger; // a logger for the entire application

	vector<std::shared_ptr<Frame>> calibrationImages;
};

