#include "Manager.h"

Manager::Manager()
{
    logger = new Logger();
    framePool = FramePool(logger);
    logger->enterLog("Manager constructed");
}

Manager::~Manager()
{
    logger->enterLog("Manager destructed");
    delete logger;
    delete& framePool;
}

vector<int> Manager::getAllSinks()
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

vector<CameraHardwareInfo> Manager::enumerateAvailableCameras()
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
        if (strncmp(entry->d_name, "video", 5) == 0) {
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

            cameras.emplace_back(
                CameraHardwareInfo{
                    .name = deviceName,
                    .path = devicePath
                }
            );
            logger->enterLog("Camera found: " + deviceName + " at " + devicePath);
        }
    }
    closedir(dir);
    return cameras;
}

bool Manager::bindSourceToSink(int sourceId, int sinkId) {
    logger->enterLog("bindSourceToSink called with sourceId=" + std::to_string(sourceId) + ", sinkId=" + std::to_string(sinkId));
    ISource* source;
    ISink* sink;

    if (sources.find(sourceId) == sources.end()) {
        logger->enterLog("Source not found: " + std::to_string(sourceId));
        return false;
    }
    source = &sources.find(sourceId)->second;

    if (sinks.find(sinkId) != sinks.end()) {
        logger->enterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }
    sink = &sinks.find(sinkId)->second;

    bool result = sink->bindSource(source);
    logger->enterLog("bindSourceToSink result: " + std::to_string(result));
    return result;
}

bool Manager::unbindSourceFromSink(int sinkId) {
    logger->enterLog("unbindSourceFromSink called with sinkId=" + std::to_string(sinkId));
    ISink* sink;

    if (sinks.find(sinkId) != sinks.end()) {
        logger->enterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }
    sink = &sinks.find(sinkId)->second;

    bool result = sink->unbindSource();
    logger->enterLog("unbindSourceFromSink result: " + std::to_string(result));
    return result;
}

int Manager::createCameraSource(CameraHardwareInfo info)
{
    logger->enterLog("createCameraSource called with name=" + info.name + ", path=" + info.path);
    return 0;
}

int Manager::createVideoFileSource(string path)
{
    logger->enterLog("createVideoFileSource called with path=" + path);
    int id = generateUUID();

    VideoFileFrameSource source = VideoFileFrameSource(logger, path, &framePool);

    sources.emplace(id, source);

    logger->enterLog("VideoFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::createImageFileSource(string path)
{
    logger->enterLog("createImageFileSource called with path=" + path);
    int id = generateUUID();

    ImageFileFrameSource source = ImageFileFrameSource(path, logger);

    sources.emplace(id, source);

    logger->enterLog("ImageFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::createApriltagSink()
{
    logger->enterLog("createApriltagSink called");
    int id = generateUUID();

    ApriltagSink sink = ApriltagSink(logger);

    sinks.emplace(id, sink);

    logger->enterLog("ApriltagSink created with id=" + std::to_string(id));
    return id;
}

int Manager::createObjectDetectionSink()
{
    logger->enterLog("createObjectDetectionSink called");
    return 0;
}

bool Manager::startAllSinks()
{
    logger->enterLog("startAllSinks called");
    return false;
}

bool Manager::stopAllSinks()
{
    logger->enterLog("stopAllSinks called");
    return false;
}

string Manager::getAllSinkStatus()
{
    logger->enterLog("getAllSinkStatus called");
    string returnString;
    returnString += "{[";

    auto iterator = sinks.begin();

    while (iterator != sinks.end()) {
        returnString += "\"";
        returnString += iterator->second.getStatus();
        returnString += "\",";
        iterator++;
    }

    returnString += "]}";

    return returnString;
}

string Manager::getSinkStatusById(int sinkId)
{
    logger->enterLog("getSinkStatusById called with sinkId=" + std::to_string(sinkId));
    ISink* sink;

    if (sinks.find(sinkId) != sinks.end()) {
        logger->enterLog("Sink not found: " + std::to_string(sinkId));
        return "";
    }
    sink = &sinks.find(sinkId)->second;

    string status = sink->getStatus();
    logger->enterLog("getSinkStatusById result: " + status);
    return status;
}

string Manager::getSinkResult(int sinkId)
{
    logger->enterLog("getSinkResult called with sinkId=" + std::to_string(sinkId));
    if (results.find(sinkId) == results.end()) {
        logger->enterLog("Result not found for sinkId: " + std::to_string(sinkId));
        return "";
    }
    string result = results.find(sinkId)->second;
    logger->enterLog("getSinkResult result: " + result);
    return result;
}

string Manager::getAllSinkResults()
{
    logger->enterLog("getAllSinkResults called");
    string returnString;
    returnString += "{[";

    auto iterator = results.begin();

    while (iterator != results.end()) {
        returnString += "{";
        returnString += iterator->second;
        returnString += "},";
        iterator++;
    }

    returnString += "]}";

    return returnString;
}

vector<Log> Manager::getAllLogs()
{
    logger->enterLog("getAllLogs called");
    return logger->GetAllLogs();
}

bool Manager::setSinkResult(int sinkId, string result)
{
    logger->enterLog("setSinkResult called with sinkId=" + std::to_string(sinkId) + ", result=" + result);
    if (results.find(sinkId) == results.end()) {
        logger->enterLog("Result entry not found for sinkId: " + std::to_string(sinkId));
        return false;
    }
    else {
        results.find(sinkId)->second = result;
        logger->enterLog("Result set for sinkId: " + std::to_string(sinkId));
        return true;
    }
}

int Manager::generateUUID()
{
    logger->enterLog("generateUUID called");
    std::mt19937 engine(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    std::uniform_int_distribution<int> dist(-2147483648, 2147483647);

    int randomNumber = dist(engine);
    logger->enterLog("Generated UUID: " + std::to_string(randomNumber));
    return 0;
}
