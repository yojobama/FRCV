//
// Created by yojobama on 6/26/25.
//
#pragma once
#ifndef FRCVCORE_H
#define FRCVCORE_H

#include <vector>
#include <string>
#include <map>
#include <random>
#include <memory>

using namespace std;
class ISink;
class CameraCalibrationResult;
class Log;
class Logger;
class FramePool;
class ISource;
class Frame;

typedef struct {
	string name;
	string path;
} CameraHardwareInfo;

class CalibrationSink;
class PreProcessor;

class FRCVCore {
public:
	FRCVCore();
	~FRCVCore();
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
	int createApriltagSink(CameraCalibrationResult calibResult);
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

	vector<std::shared_ptr<Frame>> calibrationImages;

	CalibrationSink* calibrationSink;
};

#endif //FRCVCORE_H
