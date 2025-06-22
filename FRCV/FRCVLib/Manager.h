#pragma once
#include <vector>
#include <string>
#include <map>
#include <random>
#include <chrono>
#include <thread>
#include "ISink.h"
#include "ImageFileSource.h"
#include "VideoFileSource.h"
#include "ApriltagSink.h"
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <dirent.h>

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

	bool startAllSinks();
	bool stopAllSinks();

	string getAllSinkStatus();

	string getSinkStatusById(int sinkId);

	string getSinkResult(int sinkId);
	string getAllSinkResults();

	vector<Log> getAllLogs();

	void clearAllLogs();
private:
	bool setSinkResult(int sinkId, string result);

	int generateUUID();

	// maps for storing results, sources and sinks
	map<int, string> results;
	map<int, ISource> sources;
	map<int, ISink> sinks;

	FramePool framePool;

	Logger* logger; // a logger for the entire application
};

