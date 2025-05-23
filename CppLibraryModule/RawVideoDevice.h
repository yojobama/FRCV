//
// Created by yojobama on 5/23/25.
//

#ifndef VIDEODEVICE_H
#define VIDEODEVICE_H
#include <string>
#include <vector>

struct RawVideoDevice {
    std::string devicePath;
    std::string deviceName;
};

std::vector<RawVideoDevice> ListRawVideoDevices();

#endif //VIDEODEVICE_H
