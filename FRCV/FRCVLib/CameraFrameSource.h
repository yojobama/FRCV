#pragma once
#include "IFrameSource.h"
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

class CameraFrameSource : IFrameSource 
{
public:
	CameraFrameSource(std::string devicePath);
	CameraFrameSource(std::string devicePath, std::string deviceName);
	
	~CameraFrameSource();
	
	std::string getDevicePath();
	std::string getDeviceName();
	void setDeviceName(std::string deviceName);

	Frame* getFrame();
private:
	cv::VideoCapture* capture;
	std::string devicePath;
	std::string deviceName;
};

std::vector<CameraFrameSource> enumerateCameras();