#include <cstdio>
#include <iostream>
#include "../FRCVLib/Manager.h"

using namespace std;

int main()
{
    Manager* manager = new Manager();
    
    vector<CameraHardwareInfo> cameras = manager->enumerateAvailableCameras();

    for (CameraHardwareInfo info : cameras) {
        cout << info.name << endl;
    }

    int sink = manager->createApriltagSink();

    int source = manager->createImageFileSource("/mnt/c/Users/yojob/Downloads/apriltag.png");

    manager->bindSourceToSink(source, sink);

    manager->startAllSources();
    manager->startAllSinks();

    sleep(1);

    manager->stopAllSources();
    manager->stopAllSinks();

    cout << manager->getSinkResult(sink) << endl;

    return 0;
}