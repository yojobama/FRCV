#include "Manager.h"
#include "ImageFileSource.h"
#include "VideoFileSource.h"
#include "ApriltagSink.h"
#include "RecordSink.h"
#include "CameraSource.h"
#include "Frame.h"
#include "SystemMonitor.h"

#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <dirent.h>
#include "PreProcessor.h"
#include "ISink.h"
#include "CameraCalibrationSink.h"
#include <opencv2/opencv.hpp>

Manager::Manager(string logFile)
{
    m_Logger = new Logger(logFile);
    m_FramePool = new FramePool(m_Logger);
    m_Logger->EnterLog("Manager constructed");
    m_PreProcessor = new PreProcessor(m_FramePool);
	m_SystemMonitor = new SystemMonitor(1000); // 1 second interval
	m_SystemMonitor->StartMonitoring();
}

Manager::Manager()
{
    m_Logger = new Logger("FRCVLog.txt");
    m_FramePool = new FramePool(m_Logger);
    m_Logger->EnterLog("Manager constructed");
    m_PreProcessor = new PreProcessor(m_FramePool);
	m_SystemMonitor = new SystemMonitor(1000); // 1 second interval
    m_SystemMonitor->StartMonitoring();
}

Manager::~Manager()
{
	m_SystemMonitor->StopMonitoring();
    m_Logger->EnterLog("Manager destructed");
    delete m_FramePool;
    delete m_Logger;
	delete m_PreProcessor;
    delete m_SystemMonitor;
    // Clean up sources
    for (auto& source : m_Sources) {
        delete source.second;
    }
    m_Sources.clear();
    // Clean up sinks
    for (auto& sink : m_Sinks) {
        delete sink.second;
    }
	m_Sinks.clear();
}

vector<int> Manager::GetAllSinks()
{
    m_Logger->EnterLog("GetAllSinks called");
    vector<int> returnVector;

    auto iterator = m_Sinks.begin();

    while (iterator != m_Sinks.end())
    {
        returnVector.push_back(iterator->first);
        iterator++;
    }

    return returnVector;
}

vector<int> Manager::GetAllSources()
{
	vector<int> sourceIds;

	auto iterator = m_Sources.begin();

    while (iterator != m_Sources.end()) {
		sourceIds.push_back(iterator->first);
        iterator++;
    }

    return sourceIds;
}

vector<CameraHardwareInfo> Manager::EnumerateAvailableCameras()
{
    m_Logger->EnterLog("EnumerateAvailableCameras called");
    vector<CameraHardwareInfo> cameras;
    const char* p_VideoDir = "/dev/";
    DIR* p_Dir = opendir(p_VideoDir);
    if (!p_Dir) {
        m_Logger->EnterLog("Failed to open /dev/ directory");
        return cameras;
    }

    struct dirent* p_Entry;
    while ((p_Entry = readdir(p_Dir)) != nullptr) {
        // Check if name starts with "video"
        if (strncmp(p_Entry->d_name, "video", 5) == 0) {
            // Check if the rest is an even number
            const char* p_NumPart = p_Entry->d_name + 5;
            char* p_EndPtr;
            long num = strtol(p_NumPart, &p_EndPtr, 10);
            if (*p_NumPart != '\0' && *p_EndPtr == '\0' && num % 2 == 0) {
                std::string devicePath = std::string(p_VideoDir) + p_Entry->d_name;
                int fd = open(devicePath.c_str(), O_RDONLY);
                if (fd < 0) {
                    m_Logger->EnterLog("Failed to open device: " + devicePath);
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
                m_Logger->EnterLog("Camera found: " + deviceName + " at " + devicePath);
            }
        }
    }
    closedir(p_Dir);
    return cameras;
}

bool Manager::BindSourceToSink(int sourceId, int sinkId) {
    m_Logger->EnterLog("BindSourceToSink called with sourceId=" + std::to_string(sourceId) + ", sinkId=" + std::to_string(sinkId));
    ISource* p_Source;
    ISink* p_Sink;

    if (m_Sources.find(sourceId) == m_Sources.end()) {
        m_Logger->EnterLog("Source not found: " + std::to_string(sourceId));
        return false;
    }
    p_Source = m_Sources.find(sourceId)->second;

    if (m_Sinks.find(sinkId) == m_Sinks.end()) {
        m_Logger->EnterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }
    p_Sink = m_Sinks.find(sinkId)->second;

    bool result = p_Sink->BindSource(p_Source);
    m_Logger->EnterLog("BindSourceToSink result: " + std::to_string(result));
    return result;
}

bool Manager::UnbindSourceFromSink(int sinkId) {
    m_Logger->EnterLog("UnbindSourceFromSink called with sinkId=" + std::to_string(sinkId));
    ISink* p_Sink;

    if (m_Sinks.find(sinkId) == m_Sinks.end()) {
        m_Logger->EnterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }
    p_Sink = m_Sinks.find(sinkId)->second;

    bool result = p_Sink->UnbindSource();
    m_Logger->EnterLog("UnbindSourceFromSink result: " + std::to_string(result));
    return result;
}

int Manager::CreateCameraSource(CameraHardwareInfo info)
{
    m_Logger->EnterLog("CreateCameraSource called with name=" + info.name + ", path=" + info.path);
    
    int id = GenerateUUID();

    CameraFrameSource* p_Source = new CameraFrameSource(info.path, m_Logger, m_FramePool);

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("CameraFrameSource created with id=" + std::to_string(id));
    
    return id;
}

int Manager::CreateCameraSource(CameraHardwareInfo info, int id)
{
    m_Logger->EnterLog("CreateCameraSource called with name=" + info.name + ", path=" + info.path);

    CameraFrameSource* p_Source = new CameraFrameSource(info.path, m_Logger, m_FramePool);

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("CameraFrameSource created with id=" + std::to_string(id));

    return id;
}

int Manager::CreateVideoFileSource(string path, int fps)
{
    m_Logger->EnterLog("CreateVideoFileSource called with path=" + path);
    int id = GenerateUUID();

    VideoFileFrameSource* p_Source = new VideoFileFrameSource(m_Logger, path, m_FramePool, fps);

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("VideoFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateVideoFileSource(string path, int fps, int id)
{
    m_Logger->EnterLog("CreateVideoFileSource called with path=" + path);

    VideoFileFrameSource* p_Source = new VideoFileFrameSource(m_Logger, path, m_FramePool, fps);

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("VideoFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateImageFileSource(string path)
{
    m_Logger->EnterLog("CreateImageFileSource called with path=" + path);
    int id = GenerateUUID();

    ImageFileFrameSource* p_Source = new ImageFileFrameSource(path, m_Logger, m_FramePool);

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("ImageFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateImageFileSource(string path, int id)
{
    m_Logger->EnterLog("CreateImageFileSource called with path=" + path);

    ImageFileFrameSource* p_Source = new ImageFileFrameSource(path, m_Logger, m_FramePool);

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("ImageFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateApriltagSink()
{
    m_Logger->EnterLog("CreateApriltagSink called");
    int id = GenerateUUID();

    ApriltagSink* p_Sink = new ApriltagSink(m_Logger, m_PreProcessor);

    m_Sinks.emplace(id, p_Sink);

    m_Logger->EnterLog("ApriltagSink created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateApriltagSink(int id)
{
    m_Logger->EnterLog("CreateApriltagSink called");

    ApriltagSink* p_Sink = new ApriltagSink(m_Logger, m_PreProcessor);

    m_Sinks.emplace(id, p_Sink);

    m_Logger->EnterLog("ApriltagSink created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateObjectDetectionSink()
{
    m_Logger->EnterLog("CreateObjectDetectionSink called");
    return 0;
}

int Manager::CreateObjectDetectionSink(int id)
{
    m_Logger->EnterLog("CreateObjectDetectionSink called");
    return 0;
}

int Manager::CreateRecordingSink(int sourceId)
{
    int id = GenerateUUID();

    // TODO: generate the path to the video file, an empty string will couse a faliure

    /*RecordSink* p_RecordSink = new RecordSink(m_Logger, "");

    m_Sinks.emplace(id, p_RecordSink);*/

    return id;
}

void Manager::StartAllSources()
{
    auto iterator = m_Sources.begin();

    while (iterator != m_Sources.end()) {
        iterator->second->ChangeThreadStatus(true);
        iterator++;
    }
}

void Manager::StopAllSources()
{
    auto iterator = m_Sources.begin();

    while (iterator != m_Sources.end()) {
        iterator->second->ChangeThreadStatus(false);
        iterator++;
    }
}

bool Manager::StopSourceById(int sourceId)
{
    auto source = m_Sources.find(sourceId);
    if (source == m_Sources.end()) {
        return false;
    }
    source->second->ChangeThreadStatus(false);
    return true;
}

bool Manager::StartSourceById(int sourceId)
{
    auto source = m_Sources.find(sourceId);
    if (source == m_Sources.end()) {
        return false;
    }
    source->second->ChangeThreadStatus(true);
    return true;
}

void Manager::StartAllSinks() {
    if (!m_Sinks.empty()) {
        auto iterator = m_Sinks.begin();

        while (iterator != m_Sinks.end()) {
            iterator->second->ChangeThreadStatus(true);
            iterator++;
        }
    }
}

void Manager::StopAllSinks() {
    auto iterator = m_Sinks.begin();

    while (iterator != m_Sinks.end()) {
        iterator->second->ChangeThreadStatus(false);
        iterator++;
    }
}

bool Manager::StopSinkById(int sinkId) {
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) {
        return false;
    }
    sink->second->ChangeThreadStatus(false);
    return true;
}

bool Manager::StartSinkById(int sinkId) {
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) {
        return false;
    }
    sink->second->ChangeThreadStatus(true);
    return true;
}

string Manager::GetAllSinkStatus()
{
    m_Logger->EnterLog("GetAllSinkStatus called");
    string returnString = "{";

    auto iterator = m_Sinks.begin();

    while (iterator != m_Sinks.end()) {
        returnString += "\"" + std::to_string(iterator->first) + "\": \"";
        returnString += iterator->second->GetStatus();
        returnString += "\"";

        iterator++;
        if (iterator != m_Sinks.end()) {
            returnString += ", ";
        }
    }

    returnString += "}";

    return returnString;
}

string Manager::GetSinkStatusById(int sinkId)
{
    m_Logger->EnterLog("GetSinkStatusById called with sinkId=" + std::to_string(sinkId));
    ISink* p_Sink;

    if (m_Sinks.find(sinkId) == m_Sinks.end()) {
        m_Logger->EnterLog("Sink not found: " + std::to_string(sinkId));
        return "";
    }
    p_Sink = m_Sinks.find(sinkId)->second;

    string status = p_Sink->GetStatus();
    m_Logger->EnterLog("GetSinkStatusById result: " + status);
    return status;
}

string Manager::GetSinkResult(int sinkId)
{
    m_Logger->EnterLog("GetSinkResult called with sinkId=" + std::to_string(sinkId));
    if (m_Sinks.find(sinkId) == m_Sinks.end()) {
        m_Logger->EnterLog("Result not found for sinkId: " + std::to_string(sinkId));
        return "";
    }
    string result = m_Sinks.find(sinkId)->second->GetCurrentResults();
    m_Logger->EnterLog("GetSinkResult result: " + result);
    return result;
}

string Manager::GetAllSinkResults()
{
    m_Logger->EnterLog("GetAllSinkResults called");
    string returnString = "{";

    auto iterator = m_Sinks.begin();

    while (iterator != m_Sinks.end()) {
        returnString += "\"" + std::to_string(iterator->first) + "\": \"";
        returnString += iterator->second->GetCurrentResults();
        returnString += "\"";

        iterator++;
        if (iterator != m_Sinks.end()) {
            returnString += ", ";
        }
    }

    returnString += "}";

    return returnString;
}

bool Manager::SetSinkResult(int sinkId, string result)
{
    m_Logger->EnterLog("SetSinkResult called with sinkId=" + std::to_string(sinkId) + ", result=" + result);
    if (m_Sinks.find(sinkId) == m_Sinks.end()) {
        m_Logger->EnterLog("Result entry not found for sinkId: " + std::to_string(sinkId));
        return false;
    }
    else {
        m_Sinks.find(sinkId)->second->GetCurrentResults() = result;
        m_Logger->EnterLog("Result set for sinkId: " + std::to_string(sinkId));
        return true;
    }
}

int Manager::GenerateUUID()
{
    m_Logger->EnterLog("GenerateUUID called");
    std::mt19937 engine(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    std::uniform_int_distribution<int> dist(0, 2147483647);

    int randomNumber = dist(engine);
    m_Logger->EnterLog("Generated UUID: " + std::to_string(randomNumber));
    return randomNumber;
}

int Manager::CreateCameraCalibrationSink(int width, int height)
{
	int id = GenerateUUID();

	CameraCalibrationSink* p_Sink = new CameraCalibrationSink(m_Logger, m_PreProcessor, FrameSpec(height, width, CV_8UC3));

	m_CameraCalibrationSinks.emplace(id, p_Sink);

    return 0;
}

void Manager::BindSourceToCalibrationSink(int sourceId)
{
	auto sink = m_CameraCalibrationSinks.find(sourceId);
    if (sink != m_CameraCalibrationSinks.end() && m_Sources.find(sourceId) != m_Sources.end()) {
		sink->second->BindSource(m_Sources.find(sourceId)->second);
    }
}

void Manager::CameraCalibrationSinkGrabFrame(int sinkId)
{
    auto sink = m_CameraCalibrationSinks.find(sinkId);
    if (sink != m_CameraCalibrationSinks.end()) {
        sink->second->GrabAndProcessFrame();
    } else {
        m_Logger->EnterLog("CameraCalibrationSink not found with id: " + std::to_string(sinkId));
	}
}

CameraCalibrationResult Manager::GetCameraCalibrationResults(int sinkId)
{
	auto sink = m_CameraCalibrationSinks.find(sinkId);
    if (sink != m_CameraCalibrationSinks.end()) {
		return sink->second->GetResults();
    }
    // Return empty result if not found
    return CameraCalibrationResult();
}

int Manager::GetMemoryUsageBytes()
{
	// TODO: implement a function to get memory usage
    return m_SystemMonitor->GetRAMUsage();
}

/*
	returns the CPU usage in percents
*/
int Manager::GetCPUUsage()
{
	// TODO: implement a function to get CPU usage
    return m_SystemMonitor->GetCPUUsage();
}

/*
	returns the CPU temperature in degrees Celsius 
    if you are an american, deal with it :)
*/
int Manager::GetCpuTemperature()
{
	// TODO: implement a function to get CPU temperature
    return m_SystemMonitor->GetCPUTemperature();
}

/*
   returns the disk usage in percents
*/
int Manager::GetDiskUsage()
{
    return m_SystemMonitor->GetDiskUsage();
}
