#include "CameraFrameSource.h"

CameraFrameSource::CameraFrameSource(std::string devicePath, Logger* logger, FramePool* framePool) : IFrameSource(framePool, logger)
{
    capture = new cv::VideoCapture(devicePath, cv::CAP_V4L2);
    this->devicePath = devicePath;
    this->deviceName = getDeviceName();
}

CameraFrameSource::CameraFrameSource(std::string devicePath, std::string deviceName, Logger* logger, FramePool* framePool) : IFrameSource(framePool, logger)
{
	capture = new cv::VideoCapture(devicePath, cv::CAP_V4L2);
	this->deviceName = deviceName;
	this->devicePath = devicePath;
}

CameraFrameSource::~CameraFrameSource()
{
    delete capture;
}

std::string CameraFrameSource::getDevicePath()
{
    return devicePath;
}

std::string CameraFrameSource::getDeviceName()
{
    return deviceName;
}

void CameraFrameSource::changeDeviceName(std::string newName)
{
    this->deviceName = newName;
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
                // Use the card name as the device name, replacing spaces and non-alphanumerics with underscores
                std::string rawName = reinterpret_cast<char*>(cap.card);
                std::string formattedName;
                for (char c : rawName) {
                    if (isalnum(static_cast<unsigned char>(c))) {
                        formattedName += c;
                    } else if (c == ' ' || c == '-' || c == '.') {
                        formattedName += '_';
                    }
                    // skip other non-alphanumerics
                }
                // Remove leading/trailing underscores
                size_t start = formattedName.find_first_not_of('_');
                size_t end = formattedName.find_last_not_of('_');
                if (start != std::string::npos && end != std::string::npos) {
                    formattedName = formattedName.substr(start, end - start + 1);
                }
                deviceName = formattedName;
            }
            close(fd);

            cameras.emplace_back(devicePath, deviceName);
        }
    }
    closedir(dir);
    return cameras;
}

Frame* CameraFrameSource::getFrame() {  
    Frame* frame = framePool->getFrame(frameSpec);  
    if (capture->isOpened()) {  
        logger->enterLog(INFO, "camera is open, grabbing frame and returning it");
        capture->read(*frame);
        return frame;
    }
    logger->enterLog(ERROR, "camera is closed, returning a null pointer");
    return nullptr;  
}