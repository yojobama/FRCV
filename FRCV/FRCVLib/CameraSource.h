#pragma once
#include "ISource.h"
#include <vector>
#include <string>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

namespace cv {
	class VideoCapture;
};

class CameraFrameSource : public SourceBase 
{
public:
	CameraFrameSource(std::string devicePath, Logger* logger, FramePool* framePool);
	CameraFrameSource(std::string devicePath, std::string deviceName, Logger* logger, FramePool* framePool);
	
	~CameraFrameSource();
	
	std::string getDevicePath();
	std::string getDeviceName();
	void changeDeviceName(std::string newName);

private:
	void CaptureFrame() override;
	cv::VideoCapture* capture;
	std::string m_DevicePath;
	std::string m_DeviceName;
	Logger* m_Logger;
};
