#pragma once
#include <vector>
#include <string>
#include <map>
#include "ISink.h"
#include "IFrameSource.h"

using namespace std;

// TODO: temporary, rewrite this part
typedef struct {

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
	bool bindSourceToSink(int source, int sink);
	bool unbindSourceFromSink(int source, int sink);

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
private:
	void setSinkResult(int sinkId, string result);

	// maps for storing results, sources and sinks
	map<int, string> results;
	map<int, IFrameSource> sources;
	map<int, ISink> sinks; // TODO: make the sink return a json result
};

