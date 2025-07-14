#pragma once
#include <vector>
#include <memory>
#include "FrameSpec.h"
#include <opencv2/opencv.hpp>

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
	void grabAndProcessFrame();
	CameraCalibrationResult getResults();
private:
	FrameSpec frameSpec;
	ISource* source;
	int CHECKERBOARD[2] = { 6, 9 }; // Number of inner corners per a chessboard row and column

	std::vector<std::vector<cv::Point3f> > objpoints;
	// Creating vector to store vectors of 2D points for each checkerboard image
	std::vector<std::vector<cv::Point2f> > imgpoints;
	std::vector<cv::Point3f> objp;
	std::vector<cv::Point2f> corner_pts;

	Logger* logger;
	PreProcessor* preProcessor;
};

