#pragma once
#include <vector>
#include <string>
#include <map>
#include <random>
#include <chrono>
#include <pthread.h>

using namespace std;
class ISink;
class CameraCalibrationResult;
class Log;
class Logger;
class FramePool;
class ISource;
class Frame;

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
	Manager();
	~Manager();

	// utill functions
	vector<int> getAllSinks();

	vector<CameraHardwareInfo> enumerateAvailableCameras();
	bool bindSourceToSink(int sourceId, int sinkId);
	bool unbindSourceFromSink(int sinkId);

	// functions to create frame sources
	int createCameraSource(CameraHardwareInfo info);
	int createVideoFileSource(string path);
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

	vector<string> getRecording(int recorderId); // TODO: implement a recording mechanisem

	vector<Log*> *getAllLogs();

	void clearAllLogs();

	bool takeCalibrationImage(int cameraId);
	CameraCalibrationResult conculdeCalibration();
private:
	bool setSinkResult(int sinkId, string result);

	int generateUUID();

	// maps for storing results, sources and sinks
	map<int, ISource*> sources;
	map<int, ISink*> sinks;

	FramePool *framePool;

	PreProcessor* preProcessor;

	Logger* logger; // a logger for the entire application

	vector<Frame*> calibrationImages;

	CalibrationSink* calibrationSink;
};

