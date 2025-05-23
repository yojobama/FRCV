//
// Created by yojobama on 5/23/25.
//

#include "RawVideoDevice.h"

#include <set>
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#endif

#ifdef __linux__
std::vector<RawVideoDevice> ListRawVideoDevices() {
    std::vector<RawVideoDevice> devices;
    std::set<std::string> devicePaths;
    for (int i = 0; i < 64; ++i) {
        std::string devicePath = "/dev/video" + std::to_string(i);
        if (devicePaths.contains(devicePath)) continue;

        int fd = open(devicePath.c_str(), O_RDWR);
        if (fd < 0) continue;

        v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
            close(fd);
            continue;
        }

        RawVideoDevice device;
        device.devicePath = devicePath;
        device.deviceName = reinterpret_cast<const char *>(cap.card);

        devices.push_back(device);
        devicePaths.insert(devicePath);
        close(fd);
    }
    return devices;
}
#endif
