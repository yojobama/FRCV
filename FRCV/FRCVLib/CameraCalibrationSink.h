#pragma once
#include <vector>
#include <memory>
#include "FrameSpec.h"
#include <opencv2/opencv.hpp>

using namespace std;

class SourceBase;
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

	void BindSource(SourceBase* source);
	void GrabAndProcessFrame();
	CameraCalibrationResult GetResults();
private:
	FrameSpec m_FrameSpec;
	SourceBase* m_Source;
	int CHECKERBOARD[2] = { 6, 9 }; // Number of inner corners per a chessboard row and column

	std::vector<std::vector<cv::Point3f> > m_Objpoints;
	// Creating vector to store vectors of 2D points for each checkerboard image
	std::vector<std::vector<cv::Point2f> > m_Imgpoints;
	std::vector<cv::Point3f> m_Objp;
	std::vector<cv::Point2f> M_CornerPts;

	Logger* logger;
	PreProcessor* preProcessor;
};

