#include <cstdio>
#include <iostream>
#include "../FRCVLib/Manager.h"

using namespace std;

int main()
{
    Manager manager = Manager();
    
    vector<CameraHardwareInfo> cameras = manager.enumerateAvailableCameras();

    for (CameraHardwareInfo info : cameras) {
        cout << info.name << endl;
    }

    return 0;
}