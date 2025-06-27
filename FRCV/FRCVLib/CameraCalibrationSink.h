#pragma once
#include <vector>
#include <memory>
#include "ISink.h"

using namespace std;

class ISource;
class Frame;
class CameraCalibrationResult;
class Logger;
class PreProcessor;
class FrameSpec;

class CameraCalibrationSink
{
public:
	CameraCalibrationSink(Logger* logger, PreProcessor* preProcessor, FrameSpec frameSpec);
	~CameraCalibrationSink();

	void bindSource(ISource* source);
	void grabFrame();
	CameraCalibrationResult getResults();
private:
	FrameSpec frameSpec;
	ISource* source;
	int CHECKERBOARD[2] = { 6, 9 }; // Number of inner corners per a chessboard row and column
	vector<shared_ptr<Frame>> frames;
	Logger* logger;
	PreProcessor* preProcessor;
};

