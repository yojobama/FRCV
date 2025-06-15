#include <cstdio>
#include "ApriltagSinkTest.h"
#include "FrameTest.h"
#include "FramePoolTest.h"
#include "LoggerTest.h"
#include "PreProcessorTest.h"
#include "SingleSourcePipelineTest.h"
#include "VideoFileFrameSourceTest.h"
#include "CameraFrameSourceTest.h"
#include "ISinkTest.h"
#include "IFrameSourceTest.h"
#include "ApriltagDetectionTest.h"
#include <iostream>

int main()
{
    ApriltagSinkTest apriltagTest;
    apriltagTest.doTest();

    FrameTest frameTest;
    frameTest.doTest();

    FramePoolTest framePoolTest;
    framePoolTest.doTest();

    LoggerTest loggerTest;
    loggerTest.doTest();

    PreProcessorTest preProcessorTest;
    preProcessorTest.doTest();

    SingleSourcePipelineTest singleSourcePipelineTest;
    singleSourcePipelineTest.doTest();

    VideoFileFrameSourceTest videoFileFrameSourceTest;
    videoFileFrameSourceTest.doTest();

    CameraFrameSourceTest cameraFrameSourceTest;
    cameraFrameSourceTest.doTest();

    ISinkTest isinkTest;
    isinkTest.doTest();

    IFrameSourceTest iframeSourceTest;
    iframeSourceTest.doTest();

    ApriltagDetectionTest apriltagDetectionTest;
    apriltagDetectionTest.doTest();

    return 0;
}