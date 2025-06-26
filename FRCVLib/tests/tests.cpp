//
// Created by yojobama on 6/26/25.
//
#include <iostream>
#include <string>
#include "FRCVCore.h"
#include <vector>
#include "CameraCalibrationResult.h"

using namespace std;

void* threadProcess(FRCVCore* core);
bool terminateThread = false;

int main() {
    FRCVCore* core = new FRCVCore();

    vector<CameraHardwareInfo> cameras = core->enumerateAvailableCameras();

    int source = core->createCameraSource(cameras[0]);

    for (int i = 0; i < 12; i++) {
        cin.getline(nullptr, 0);
        core->takeCalibrationImage(source);
        printf("frame taken\n");
    }
    cin.getline(nullptr, 0);

    CameraCalibrationResult result = core->conculdeCalibration();

    int sink = core->createApriltagSink(result);

    printf("Hello World!\n");
}