#include "CameraFrameSource.h"

CameraFrameSource::CameraFrameSource(std::string devicePath)
{
	capture = new cv::VideoCapture(devicePath, cv::CAP_V4L2);
	this->devicePath = devicePath;
	this->deviceName = getDeviceName();
}

CameraFrameSource::CameraFrameSource(std::string devicePath, std::string deviceName)
{
	capture = new cv::VideoCapture(devicePath, cv::CAP_V4L2);
	this->deviceName = deviceName;
	this->devicePath = devicePath;
}


std::vector<CameraFrameSource> enumerateCameras()
{
    std::vector<CameraFrameSource> cameras;
    const char* video_dir = "/dev/";
    DIR* dir = opendir(video_dir);
    if (!dir) return cameras;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "video", 5) == 0) {
            std::string devicePath = std::string(video_dir) + entry->d_name;
            int fd = open(devicePath.c_str(), O_RDONLY);
            if (fd < 0) continue;

            struct v4l2_capability cap;
            std::string deviceName = devicePath;
            if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
                deviceName = reinterpret_cast<char*>(cap.card);
            }
            close(fd);

            cameras.emplace_back(devicePath, deviceName);
        }
    }
    closedir(dir);
    return cameras;
}
