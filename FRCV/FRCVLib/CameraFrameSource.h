#pragma once
#include "IFrameSource.h"
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

class CameraFrameSource : public IFrameSource 
{
public:
	CameraFrameSource(std::string devicePath, Logger* logger, FramePool* framePool);
	CameraFrameSource(std::string devicePath, std::string deviceName, Logger* logger, FramePool* framePool);
	
	~CameraFrameSource();
	
	std::string getDevicePath();
	std::string getDeviceName();
	void changeDeviceName(std::string newName);

	Frame* getFrame();
private:
	cv::VideoCapture* capture;
	std::string devicePath;
	std::string deviceName;
};

std::vector<CameraFrameSource> enumerateCameras();