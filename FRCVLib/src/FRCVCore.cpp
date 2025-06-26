#include "FRCVCore.h"

#include "Sources/ImageFileSource.h"
#include "Sources/VideoFileSource.h"
#include "Sinks/ApriltagSink.h"
#include "CameraCalibrationResult.h"
#include "Sinks/RecordSink.h"
#include "Sources/CameraSource.h"

#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <dirent.h>

#include "CameraCalibrationPipeline.h"
#include "PreProcessor.h"
#include "Sinks/ISink.h"

#include "CameraCalibrationResult.h"

FRCVCore::FRCVCore()
{
    logger = new Logger();
    framePool = new FramePool(logger);
    logger->enterLog("Manager constructed");
    preProcessor = new PreProcessor(framePool);
}

FRCVCore::~FRCVCore()
{
    logger->enterLog("Manager destructed");
    delete framePool;
    delete logger;
}

vector<int> FRCVCore::getAllSinks()
{
    logger->enterLog("getAllSinks called");
    vector<int> returnVector;

    auto iterator = sinks.begin();

    while (iterator != sinks.end())
    {
        returnVector.push_back(iterator->first);
        iterator++;
    }

    return returnVector;
}

vector<CameraHardwareInfo> FRCVCore::enumerateAvailableCameras()
{
    logger->enterLog("enumerateAvailableCameras called");
    vector<CameraHardwareInfo> cameras;
    const char* video_dir = "/dev/";
    DIR* dir = opendir(video_dir);
    if (!dir) {
        logger->enterLog("Failed to open /dev/ directory");
        return cameras;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Check if name starts with "video"
        if (strncmp(entry->d_name, "video", 5) == 0) {
            // Check if the rest is an even number
            const char* numPart = entry->d_name + 5;
            char* endptr;
            long num = strtol(numPart, &endptr, 10);
            if (*numPart != '\0' && *endptr == '\0' && num % 2 == 0) {
                std::string devicePath = std::string(video_dir) + entry->d_name;
                int fd = open(devicePath.c_str(), O_RDONLY);
                if (fd < 0) {
                    logger->enterLog("Failed to open device: " + devicePath);
                    continue;
                }
                struct v4l2_capability cap;
                std::string deviceName = devicePath;
                if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
                    std::string rawName = reinterpret_cast<char*>(cap.card);
                    std::string formattedName;
                    for (char c : rawName) {
                        if (isalnum(static_cast<unsigned char>(c))) {
                            formattedName += c;
                        }
                        else if (c == ' ' || c == '-' || c == '.') {
                            formattedName += '_';
                        }
                    }
                    size_t start = formattedName.find_first_not_of('_');
                    size_t end = formattedName.find_last_not_of('_');
                    if (start != std::string::npos && end != std::string::npos) {
                        formattedName = formattedName.substr(start, end - start + 1);
                    }
                    deviceName = formattedName;
                }
                close(fd);

                cameras.push_back(
                    CameraHardwareInfo{
                        .name = deviceName,
                        .path = devicePath
                    }
                );
                logger->enterLog("Camera found: " + deviceName + " at " + devicePath);
            }
        }
    }
    closedir(dir);
    return cameras;
}

bool FRCVCore::bindSourceToSink(int sourceId, int sinkId) {
    logger->enterLog("bindSourceToSink called with sourceId=" + std::to_string(sourceId) + ", sinkId=" + std::to_string(sinkId));
    ISource* source;
    ISink* sink;

    if (sources.find(sourceId) == sources.end()) {
        logger->enterLog("Source not found: " + std::to_string(sourceId));
        return false;
    }
    source = sources.find(sourceId)->second;

    if (sinks.find(sinkId) == sinks.end()) {
        logger->enterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }
    sink = sinks.find(sinkId)->second;

    bool result = sink->bindSource(source);
    logger->enterLog("bindSourceToSink result: " + std::to_string(result));
    return result;
}

bool FRCVCore::unbindSourceFromSink(int sinkId) {
    logger->enterLog("unbindSourceFromSink called with sinkId=" + std::to_string(sinkId));
    ISink* sink;

    if (sinks.find(sinkId) != sinks.end()) {
        logger->enterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }
    sink = sinks.find(sinkId)->second;

    bool result = sink->unbindSource();
    logger->enterLog("unbindSourceFromSink result: " + std::to_string(result));
    return result;
}

int FRCVCore::createCameraSource(CameraHardwareInfo info)
{
    logger->enterLog("createCameraSource called with name=" + info.name + ", path=" + info.path);

    int id = generateUUID();

    CameraFrameSource* source = new CameraFrameSource(info.path, logger, framePool);

    sources.emplace(id, source);

    logger->enterLog("CameraFrameSource created with id=" + std::to_string(id));

    return id;
}

int FRCVCore::createVideoFileSource(string path, int fps)
{
    logger->enterLog("createVideoFileSource called with path=" + path);
    int id = generateUUID();

    VideoFileFrameSource* source = new VideoFileFrameSource(logger, path, framePool, fps);

    sources.emplace(id, source);

    logger->enterLog("VideoFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int FRCVCore::createImageFileSource(string path)
{
    logger->enterLog("createImageFileSource called with path=" + path);
    int id = generateUUID();

    ImageFileFrameSource* source = new ImageFileFrameSource(path, logger, framePool);

    sources.emplace(id, source);

    logger->enterLog("ImageFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int FRCVCore::createApriltagSink(CameraCalibrationResult calibResult)
{
    logger->enterLog("createApriltagSink called");
    int id = generateUUID();

    ApriltagSink* sink = new ApriltagSink(logger, preProcessor);

    sinks.emplace(id, sink);

    logger->enterLog("ApriltagSink created with id=" + std::to_string(id));
    return id;
}

int FRCVCore::createObjectDetectionSink()
{
    logger->enterLog("createObjectDetectionSink called");
    return 0;
}

int FRCVCore::createRecordingSink(int sourceId)
{
    int id = generateUUID();

    // TODO: generate the path to the video file, an empty string will couse a faliure

    /*RecordSink* recordSink = new RecordSink(logger, "");

    sinks.emplace(id, recordSink);*/

    return id;
}

void FRCVCore::startAllSources()
{
    auto iterator = sources.begin();

    while (iterator != sources.end()) {
        iterator->second->changeThreadStatus(true);
        iterator++;
    }
}

void FRCVCore::stopAllSources()
{
    auto iterator = sources.begin();

    while (iterator != sources.end()) {
        iterator->second->changeThreadStatus(false);
        iterator++;
    }
}

bool FRCVCore::stopSourceById(int sourceId)
{
    auto source = sources.find(sourceId);
    if (source == sources.end()) {
        return false;
    }
    source->second->changeThreadStatus(false);
    return true;
}

bool FRCVCore::startSourceById(int sourceId)
{
    auto source = sources.find(sourceId);
    if (source == sources.end()) {
        return false;
    }
    source->second->changeThreadStatus(true);
    return true;
}

void FRCVCore::startAllSinks() {
    auto iterator = sinks.begin();

    while (iterator != sinks.end()) {
        iterator->second->changeThreadStatus(true);
        iterator++;
    }
}

void FRCVCore::stopAllSinks() {
    auto iterator = sinks.begin();

    while (iterator != sinks.end()) {
        iterator->second->changeThreadStatus(false);
        iterator++;
    }
}

bool FRCVCore::stopSinkById(int sinkId) {
    auto sink = sinks.find(sinkId);
    if (sink == sinks.end()) {
        return false;
    }
    sink->second->changeThreadStatus(false);
    return true;
}

bool FRCVCore::startSinkById(int sinkId) {
    auto sink = sinks.find(sinkId);
    if (sink == sinks.end()) {
        return false;
    }
    sink->second->changeThreadStatus(true);
    return true;
}

string FRCVCore::getAllSinkStatus()
{
    logger->enterLog("getAllSinkStatus called");
    string returnString;
    returnString += "{[";

    auto iterator = sinks.begin();

    while (iterator != sinks.end()) {
        returnString += "\"";
        returnString += iterator->second->getStatus();
        returnString += "\",";
        iterator++;
    }

    returnString += "]}";

    return returnString;
}

string FRCVCore::getSinkStatusById(int sinkId)
{
    logger->enterLog("getSinkStatusById called with sinkId=" + std::to_string(sinkId));
    ISink* sink;

    if (sinks.find(sinkId) != sinks.end()) {
        logger->enterLog("Sink not found: " + std::to_string(sinkId));
        return "";
    }
    sink = sinks.find(sinkId)->second;

    string status = sink->getStatus();
    logger->enterLog("getSinkStatusById result: " + status);
    return status;
}

string FRCVCore::getSinkResult(int sinkId)
{
    logger->enterLog("getSinkResult called with sinkId=" + std::to_string(sinkId));
    if (sinks.find(sinkId) == sinks.end()) {
        logger->enterLog("Result not found for sinkId: " + std::to_string(sinkId));
        return "";
    }
    string result = sinks.find(sinkId)->second->getCurrentResults();
    logger->enterLog("getSinkResult result: " + result);
    return result;
}

string FRCVCore::getAllSinkResults()
{
    logger->enterLog("getAllSinkResults called");
    string returnString;
    returnString += "{[";
    auto iterator = sinks.begin();

    while (iterator != sinks.end()) {
        returnString += "{";
        returnString += iterator->second->getCurrentResults();
        returnString += "},";
        iterator++;
    }


    returnString += "]}";

    return returnString;
}

vector<Log*> *FRCVCore::getAllLogs()
{
    logger->enterLog("getAllLogs called");
    return logger->GetAllLogs();
}

bool FRCVCore::setSinkResult(int sinkId, string result)
{
    logger->enterLog("setSinkResult called with sinkId=" + std::to_string(sinkId) + ", result=" + result);
    if (sinks.find(sinkId) == sinks.end()) {
        logger->enterLog("Result entry not found for sinkId: " + std::to_string(sinkId));
        return false;
    }
    else {
        sinks.find(sinkId)->second->getCurrentResults() = result;
        logger->enterLog("Result set for sinkId: " + std::to_string(sinkId));
        return true;
    }
}

int FRCVCore::generateUUID()
{
    logger->enterLog("generateUUID called");
    std::mt19937 engine(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    std::uniform_int_distribution<int> dist(-2147483648, 2147483647);

    int randomNumber = dist(engine);
    logger->enterLog("Generated UUID: " + std::to_string(randomNumber));
    return randomNumber;
}

bool FRCVCore::takeCalibrationImage(int cameraId)
{
    if (sources.find(cameraId) == sources.end()) {
        return false;
    }
    calibrationImages.push_back(sources.find(cameraId)->second->getLatestFrame());
    return true;
}

CameraCalibrationResult FRCVCore::conculdeCalibration()
{
    calibrationImages.clear();
    CameraCalibrationPipeline pipeline(preProcessor, framePool);
    int checkerboard[2] = {6, 9};
    return pipeline.getResults(calibrationImages, checkerboard);
}

void FRCVCore::clearAllLogs()
{
    logger->enterLog("clearAllLogs called");
    logger->clearAllLogs();
}
