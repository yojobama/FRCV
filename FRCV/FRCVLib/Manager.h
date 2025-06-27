#pragma once
#include <vector>
#include <string>
#include <map>
#include <random>
#include <chrono>
#include <pthread.h>
#include <memory>

using namespace std;
class ISink;
class CameraCalibrationResult;
class Log;
class Logger;
class FramePool;
class ISource;
class Frame;
class CameraCalibrationSink;

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

class CalibrationSink;
class PreProcessor;

class Manager
{
public:
	Manager(string logDir);
	~Manager();

	// utill functions
	vector<int> getAllSinks();

	vector<CameraHardwareInfo> enumerateAvailableCameras();
	bool bindSourceToSink(int sourceId, int sinkId);
	bool unbindSourceFromSink(int sinkId);

	// functions to create frame sources
	int createCameraSource(CameraHardwareInfo info);
	int createVideoFileSource(string path, int fps);
	int createImageFileSource(string path);

	// functions to create detection sinks
	int createApriltagSink();
	int createObjectDetectionSink();

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

	vector<unique_ptr<Log>> getAllLogs();

	void clearAllLogs();

	int createCameraCalibrationSink();
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

	CalibrationSink* calibrationSink;
};

