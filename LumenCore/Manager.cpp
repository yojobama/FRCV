#include "Manager.h"
#include "ImageFileSource.h"
#include "VideoFileSource.h"
#include "ApriltagDetector.h"
#include "CameraCalibrator.h"
#include "StereoCalibrator.h"
#include "StereoDepthNode.h"
#include "DepthFusionNode.h"
#include "IStereoRoleReceiver.h"
#include "RecordSink.h"
#include "CameraSource.h"
#include "SystemMonitor.h"
#include "ISink.h"
#include "ObjectDetectionSink.h"
#ifdef FRCV_WITH_ONNX
#include "OnnxDetectionBackend.h"
#endif
#ifdef FRCV_WITH_NT4
#include "NetworkTablesSink.h"
#endif
#ifdef FRCV_WITH_WEBRTC
#include "WebRTCSink.h"
#endif

#include <cstring>
#include <cctype>
#include <stdexcept>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <dirent.h>
#include <opencv2/opencv.hpp>

Manager::Manager(string logFile)
{
    m_Logger = std::make_shared<Logger>(logFile);
    m_Logger->EnterLog("Manager constructed");
	m_SystemMonitor = new SystemMonitor(1000); // 1 second interval
	m_SystemMonitor->StartMonitoring();
}

Manager::Manager()
{
    m_Logger = std::make_shared<Logger>("FRCVLog.txt");
    m_Logger->EnterLog("Manager constructed");
	m_SystemMonitor = new SystemMonitor(1000); // 1 second interval
    m_SystemMonitor->StartMonitoring();
}

Manager::~Manager()
{
	m_SystemMonitor->StopMonitoring();
    m_Logger->EnterLog("Manager destructed");
    delete m_SystemMonitor;
    //m_CameraCalibrationSinks.clear();
    m_Sources.clear();
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

std::vector<std::string> Manager::GetAvailableVideoEncoders()
{
    //return FFmpegUtils::GetAvailableVideoEncoders();
    throw std::runtime_error("GetAvailableVideoEncoders not implemented yet");
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

bool Manager::BindStereoSources(int sinkId, int leftSourceId, int rightSourceId) {
    m_Logger->EnterLog("BindStereoSources called with sinkId=" + std::to_string(sinkId) +
        ", leftSourceId=" + std::to_string(leftSourceId) + ", rightSourceId=" + std::to_string(rightSourceId));

    auto sinkIt = m_Sinks.find(sinkId);
    if (sinkIt == m_Sinks.end()) {
        m_Logger->EnterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }
    auto leftIt = m_Sources.find(leftSourceId);
    auto rightIt = m_Sources.find(rightSourceId);
    if (leftIt == m_Sources.end() || rightIt == m_Sources.end()) {
        m_Logger->EnterLog("BindStereoSources: left or right source not found");
        return false;
    }

    IStereoRoleReceiver* p_RoleReceiver = dynamic_cast<IStereoRoleReceiver*>(sinkIt->second.get());
    if (p_RoleReceiver == nullptr) {
        m_Logger->EnterLog(LogLevel::Error, "BindStereoSources: sink " + std::to_string(sinkId) + " is not a stereo node");
        return false;
    }

    bool bound = sinkIt->second->BindSource(leftIt->second) && sinkIt->second->BindSource(rightIt->second);
    if (bound) {
        // recorded explicitly by source ID rather than relying on ISink::BindSource's own
        // bind-order bookkeeping, which has no left/right notion at all - see IStereoRoleReceiver.h
        p_RoleReceiver->SetStereoRoles(leftIt->second->GetID(), rightIt->second->GetID());
    }
    m_Logger->EnterLog("BindStereoSources result: " + std::to_string(bound));
    return bound;
}

bool Manager::BindSourceToSink(int sourceId, int sinkId) {
    m_Logger->EnterLog("BindSourceToSink called with sourceId=" + std::to_string(sourceId) + ", sinkId=" + std::to_string(sinkId));
    auto source = m_Sources.find(sourceId);
    if (source == m_Sources.end()) {
        m_Logger->EnterLog("Source not found: " + std::to_string(sourceId));
        return false;
    }
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) {
        m_Logger->EnterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }

    bool result = sink->second->BindSource(source->second);
    m_Logger->EnterLog("BindSourceToSink result: " + std::to_string(result));
    return result;
}

bool Manager::UnbindSourceFromSink(int sinkId) {
    m_Logger->EnterLog("UnbindSourceFromSink called with sinkId=" + std::to_string(sinkId));
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) {
        m_Logger->EnterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }

    for (const auto& sourcePair : m_Sources) {
        if (sink->second->UnbindSource(sourcePair.second->GetID())) {
            m_Logger->EnterLog("UnbindSourceFromSink result: true");
            return true;
        }
    }

    bool result = false;
    m_Logger->EnterLog("UnbindSourceFromSink result: " + std::to_string(result));
    return result;
}

bool Manager::DeleteSink(int sinkId)
{
    m_Logger->EnterLog("DeleteSink called with sinkId=" + std::to_string(sinkId));
    auto sinkIt = m_Sinks.find(sinkId);
    if (sinkIt == m_Sinks.end()) {
        m_Logger->EnterLog("Sink not found: " + std::to_string(sinkId));
        return false;
    }

    std::shared_ptr<ISink> p_Sink = sinkIt->second;
    p_Sink->Toggle(false);

    // dual-role nodes (e.g. CameraCalibrator, ApriltagDetector) are registered under the same id
    // in m_Sources too; stop that half and unbind every other sink from it before erasing either,
    // so nothing is left holding a dangling reference
    auto sourceIt = m_Sources.find(sinkId);
    if (sourceIt != m_Sources.end() && sourceIt->second == std::dynamic_pointer_cast<ISource>(p_Sink)) {
        sourceIt->second->Toggle(false);
        string sourceStringId = sourceIt->second->GetID();
        for (auto& otherSinkPair : m_Sinks) {
            if (otherSinkPair.first != sinkId) {
                otherSinkPair.second->UnbindSource(sourceStringId);
            }
        }
        m_Sources.erase(sourceIt);
    }

    m_Sinks.erase(sinkIt);
    m_Logger->EnterLog("DeleteSink removed sinkId=" + std::to_string(sinkId));
    return true;
}

bool Manager::DeleteSource(int sourceId)
{
    m_Logger->EnterLog("DeleteSource called with sourceId=" + std::to_string(sourceId));
    auto sourceIt = m_Sources.find(sourceId);
    if (sourceIt == m_Sources.end()) {
        m_Logger->EnterLog("Source not found: " + std::to_string(sourceId));
        return false;
    }

    std::shared_ptr<ISource> p_Source = sourceIt->second;
    string sourceStringId = p_Source->GetID();
    for (auto& sinkPair : m_Sinks) {
        sinkPair.second->UnbindSource(sourceStringId);
    }
    p_Source->Toggle(false);

    // dual-role nodes are registered under the same id in m_Sinks too; stop that half as well
    auto sinkIt = m_Sinks.find(sourceId);
    if (sinkIt != m_Sinks.end() && sinkIt->second == std::dynamic_pointer_cast<ISink>(p_Source)) {
        sinkIt->second->Toggle(false);
        m_Sinks.erase(sinkIt);
    }

    m_Sources.erase(sourceIt);
    m_Logger->EnterLog("DeleteSource removed sourceId=" + std::to_string(sourceId));
    return true;
}

int Manager::CreateCameraSource(CameraHardwareInfo info)
{
    m_Logger->EnterLog("CreateCameraSource called with name=" + info.name + ", path=" + info.path);
    
    int id = GenerateUUID();

    auto p_Source = std::make_shared<CameraFrameSource>(info.path, info.name, m_Logger, std::to_string(id));

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("CameraFrameSource created with id=" + std::to_string(id));
    
    return id;
}

int Manager::CreateCameraSource(CameraHardwareInfo info, int id)
{
    m_Logger->EnterLog("CreateCameraSource called with name=" + info.name + ", path=" + info.path);

    auto p_Source = std::make_shared<CameraFrameSource>(info.path, info.name, m_Logger, std::to_string(id));

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("CameraFrameSource created with id=" + std::to_string(id));

    return id;
}

int Manager::CreateVideoFileSource(string path, int fps)
{
    m_Logger->EnterLog("CreateVideoFileSource called with path=" + path);
    int id = GenerateUUID();

    auto p_Source = std::make_shared<VideoFileFrameSource>(m_Logger, path, fps, std::to_string(id));

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("VideoFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateVideoFileSource(string path, int fps, int id)
{
    m_Logger->EnterLog("CreateVideoFileSource called with path=" + path);

    auto p_Source = std::make_shared<VideoFileFrameSource>(m_Logger, path, fps, std::to_string(id));

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("VideoFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateImageFileSource(string path)
{
    m_Logger->EnterLog("CreateImageFileSource called with path=" + path);
    int id = GenerateUUID();

    auto p_Source = std::make_shared<ImageFileFrameSource>(path, m_Logger, std::to_string(id));

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("ImageFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateImageFileSource(string path, int id)
{
    m_Logger->EnterLog("CreateImageFileSource called with path=" + path);

    auto p_Source = std::make_shared<ImageFileFrameSource>(path, m_Logger, std::to_string(id));

    m_Sources.emplace(id, p_Source);

    m_Logger->EnterLog("ImageFileFrameSource created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateApriltagDetector(CameraCalibrationResult calibrationResult, double tagSize /* in METERS you filthy Americans! */)
{
    m_Logger->EnterLog("CreateApriltagDetector called");
    int id = GenerateUUID();

	auto p_Sink = std::make_shared<ApriltagDetector>(m_Logger, std::to_string(id), calibrationResult, tagSize); // TODO: add calibration result and tagSize variables to the constructor

    m_Sinks.emplace(id, p_Sink);

    m_Logger->EnterLog("ApriltagDetector created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateApriltagDetector(int id, CameraCalibrationResult calibrationResult, double tagSize /* in METERS you filthy Americans! */)
{
    m_Logger->EnterLog("CreateApriltagDetector called");

    auto p_Detector = std::make_shared<ApriltagDetector>(m_Logger, std::to_string(id), calibrationResult, tagSize); // TODO: add calibration result and tagSize variables to the constructor

    // ApriltagDetector is both an ISink (consumes camera frames) and an ISource (produces detections),
    // so it must be registered in both maps to be reachable from either side
    m_Sinks.emplace(id, p_Detector);
    m_Sources.emplace(id, p_Detector);

    m_Logger->EnterLog("ApriltagDetector created with id=" + std::to_string(id));
    return id;
}

namespace {
    constexpr double DEFAULT_APRILTAG_SIZE_METERS = 0.1651; // default FRC AprilTag size (6.5 inches), used until real calibration data is supplied
}

int Manager::CreateApriltagDetector()
{
    int id = GenerateUUID();
    return CreateApriltagDetector(id);
}

int Manager::CreateApriltagDetector(int id)
{
    return CreateApriltagDetector(id, CameraCalibrationResult(), DEFAULT_APRILTAG_SIZE_METERS);
}

int Manager::CreateApriltagDetector(CameraCalibrationResult calibrationResult, double tagSize,
    ApriltagBackendKind backendKind, int frameWidth, int frameHeight)
{
    int id = GenerateUUID();
    return CreateApriltagDetector(id, calibrationResult, tagSize, backendKind, frameWidth, frameHeight);
}

int Manager::CreateApriltagDetector(int id, CameraCalibrationResult calibrationResult, double tagSize,
    ApriltagBackendKind backendKind, int frameWidth, int frameHeight)
{
    m_Logger->EnterLog("CreateApriltagDetector called with id=" + std::to_string(id) + ", backend=" + std::to_string(backendKind));

    auto p_Detector = std::make_shared<ApriltagDetector>(m_Logger, std::to_string(id), calibrationResult, tagSize,
        backendKind, frameWidth, frameHeight);

    m_Sinks.emplace(id, p_Detector);
    m_Sources.emplace(id, p_Detector);
    return id;
}

string Manager::GetApriltagDetectorBackendName(int sinkId)
{
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) return "";

    ApriltagDetector* p_Detector = dynamic_cast<ApriltagDetector*>(sink->second.get());
    if (p_Detector == nullptr) return "";

    return p_Detector->GetBackendName();
}

namespace {
	CameraCalibrator* FindCalibrator(map<int, std::shared_ptr<ISink>>& sinks, int calibratorId)
	{
		auto sink = sinks.find(calibratorId);
		if (sink == sinks.end()) return nullptr;
		return dynamic_cast<CameraCalibrator*>(sink->second.get());
	}
}

int Manager::CreateCameraCalibrator()
{
	int id = GenerateUUID();
	return CreateCameraCalibrator(id);
}

int Manager::CreateCameraCalibrator(int id)
{
	m_Logger->EnterLog("CreateCameraCalibrator called with id=" + std::to_string(id));

	auto p_Calibrator = std::make_shared<CameraCalibrator>(m_Logger, std::to_string(id));

	// CameraCalibrator is both an ISink (consumes calibration frames) and an ISource (can feed calibrated
	// frames onward), so it must be registered in both maps to be reachable from either side
	m_Sinks.emplace(id, p_Calibrator);
	m_Sources.emplace(id, p_Calibrator);

	m_Logger->EnterLog("CameraCalibrator created with id=" + std::to_string(id));
	return id;
}

int Manager::CreateCameraCalibrator(CalibrationBoardType boardType, int rows, int cols,
	float squareSizeMeters, float markerSizeMeters, int arucoDictionaryId)
{
	int id = GenerateUUID();
	return CreateCameraCalibrator(id, boardType, rows, cols, squareSizeMeters, markerSizeMeters, arucoDictionaryId);
}

int Manager::CreateCameraCalibrator(int id, CalibrationBoardType boardType, int rows, int cols,
	float squareSizeMeters, float markerSizeMeters, int arucoDictionaryId)
{
	m_Logger->EnterLog("CreateCameraCalibrator called with id=" + std::to_string(id) + ", boardType=" + std::to_string(boardType));

	CalibrationBoardConfig config;
	config.type = boardType;
	config.rows = rows;
	config.cols = cols;
	config.squareSizeMeters = squareSizeMeters;
	config.markerSizeMeters = markerSizeMeters;
	config.arucoDictionaryId = arucoDictionaryId;

	auto p_Calibrator = std::make_shared<CameraCalibrator>(m_Logger, std::to_string(id), config);
	m_Sinks.emplace(id, p_Calibrator);
	m_Sources.emplace(id, p_Calibrator);
	return id;
}

CameraCalibrationResult Manager::GetCameraCalibrationResult(int calibratorId)
{
	CameraCalibrator* p_Calibrator = FindCalibrator(m_Sinks, calibratorId);
	if (p_Calibrator == nullptr) {
		m_Logger->EnterLog("CameraCalibrator not found: " + std::to_string(calibratorId));
		return CameraCalibrationResult();
	}
	return p_Calibrator->GetCalibrationResult();
}

CameraCalibrationResult Manager::RunCameraCalibration(int calibratorId)
{
	CameraCalibrator* p_Calibrator = FindCalibrator(m_Sinks, calibratorId);
	if (p_Calibrator == nullptr) {
		throw std::runtime_error("CameraCalibrator not found: " + std::to_string(calibratorId));
	}
	return p_Calibrator->RunCalibration();
}

int Manager::GetCameraCalibrationSnapshotCount(int calibratorId)
{
	CameraCalibrator* p_Calibrator = FindCalibrator(m_Sinks, calibratorId);
	return p_Calibrator == nullptr ? 0 : p_Calibrator->GetSnapshotCount();
}

bool Manager::RemoveCameraCalibrationSnapshot(int calibratorId, int index)
{
	CameraCalibrator* p_Calibrator = FindCalibrator(m_Sinks, calibratorId);
	return p_Calibrator != nullptr && p_Calibrator->RemoveSnapshot(index);
}

void Manager::ClearCameraCalibrationSnapshots(int calibratorId)
{
	CameraCalibrator* p_Calibrator = FindCalibrator(m_Sinks, calibratorId);
	if (p_Calibrator != nullptr) p_Calibrator->ClearSnapshots();
}

bool Manager::SaveCameraCalibrationBoardDetection(int calibratorId)
{
	CameraCalibrator* p_Calibrator = FindCalibrator(m_Sinks, calibratorId);
	if (p_Calibrator == nullptr) {
		m_Logger->EnterLog("CameraCalibrator not found: " + std::to_string(calibratorId));
		return false;
	}
	return p_Calibrator->SaveBoardDetection();
}

int Manager::CreateApriltagDetectorFromCalibrator(int calibratorId, double tagSize /* in METERS you filthy Americans! */)
{
	int id = GenerateUUID();
	return CreateApriltagDetectorFromCalibrator(id, calibratorId, tagSize);
}

int Manager::CreateApriltagDetectorFromCalibrator(int id, int calibratorId, double tagSize /* in METERS you filthy Americans! */)
{
	m_Logger->EnterLog("CreateApriltagDetectorFromCalibrator called with calibratorId=" + std::to_string(calibratorId));

	// pull the calibration result out of the CameraCalibrator sink and hand it to the new ApriltagDetector
	// so it can resolve the tag's real world location
	CameraCalibrationResult calibrationResult = GetCameraCalibrationResult(calibratorId);

	return CreateApriltagDetector(id, calibrationResult, tagSize);
}

namespace {
    // legacy default, matching CameraCalibrator's own default board
    const StereoCalibrationBoardConfig DEFAULT_STEREO_BOARD_CONFIG;

    StereoCalibrator* FindStereoCalibrator(map<int, std::shared_ptr<ISink>>& sinks, int calibratorId)
    {
        auto sink = sinks.find(calibratorId);
        if (sink == sinks.end()) return nullptr;
        return dynamic_cast<StereoCalibrator*>(sink->second.get());
    }

    StereoDepthNode* FindStereoDepthNode(map<int, std::shared_ptr<ISink>>& sinks, int nodeId)
    {
        auto sink = sinks.find(nodeId);
        if (sink == sinks.end()) return nullptr;
        return dynamic_cast<StereoDepthNode*>(sink->second.get());
    }
}

int Manager::CreateStereoCalibrator()
{
    int id = GenerateUUID();
    return CreateStereoCalibrator(id);
}

int Manager::CreateStereoCalibrator(int id)
{
    m_Logger->EnterLog("CreateStereoCalibrator called with id=" + std::to_string(id));

    auto p_Calibrator = std::make_shared<StereoCalibrator>(m_Logger, std::to_string(id));

    // like CameraCalibrator, dual-role: both an ISink (consumes the left/right camera pair) and
    // an ISource (produces the side-by-side rectified preview + status JSON)
    m_Sinks.emplace(id, p_Calibrator);
    m_Sources.emplace(id, p_Calibrator);

    m_Logger->EnterLog("StereoCalibrator created with id=" + std::to_string(id));
    return id;
}

int Manager::CreateStereoCalibrator(CalibrationBoardType boardType, int rows, int cols, float squareSizeMeters)
{
    int id = GenerateUUID();
    return CreateStereoCalibrator(id, boardType, rows, cols, squareSizeMeters);
}

int Manager::CreateStereoCalibrator(int id, CalibrationBoardType boardType, int rows, int cols, float squareSizeMeters)
{
    m_Logger->EnterLog("CreateStereoCalibrator called with id=" + std::to_string(id) + ", boardType=" + std::to_string(boardType));

    StereoCalibrationBoardConfig config;
    config.type = boardType;
    config.rows = rows;
    config.cols = cols;
    config.squareSizeMeters = squareSizeMeters;

    auto p_Calibrator = std::make_shared<StereoCalibrator>(m_Logger, std::to_string(id), config);
    m_Sinks.emplace(id, p_Calibrator);
    m_Sources.emplace(id, p_Calibrator);
    return id;
}

bool Manager::SaveStereoCalibrationDetection(int calibratorId)
{
    StereoCalibrator* p_Calibrator = FindStereoCalibrator(m_Sinks, calibratorId);
    if (p_Calibrator == nullptr) {
        m_Logger->EnterLog("StereoCalibrator not found: " + std::to_string(calibratorId));
        return false;
    }
    return p_Calibrator->SaveStereoDetection();
}

int Manager::GetStereoCalibrationPairCount(int calibratorId)
{
    StereoCalibrator* p_Calibrator = FindStereoCalibrator(m_Sinks, calibratorId);
    return p_Calibrator == nullptr ? 0 : p_Calibrator->GetPairCount();
}

bool Manager::RemoveStereoCalibrationPair(int calibratorId, int index)
{
    StereoCalibrator* p_Calibrator = FindStereoCalibrator(m_Sinks, calibratorId);
    return p_Calibrator != nullptr && p_Calibrator->RemovePair(index);
}

void Manager::ClearStereoCalibrationPairs(int calibratorId)
{
    StereoCalibrator* p_Calibrator = FindStereoCalibrator(m_Sinks, calibratorId);
    if (p_Calibrator != nullptr) p_Calibrator->ClearPairs();
}

StereoCalibrationResult Manager::RunStereoCalibration(int calibratorId)
{
    StereoCalibrator* p_Calibrator = FindStereoCalibrator(m_Sinks, calibratorId);
    if (p_Calibrator == nullptr) {
        throw std::runtime_error("StereoCalibrator not found: " + std::to_string(calibratorId));
    }
    return p_Calibrator->RunCalibration();
}

StereoCalibrationResult Manager::GetStereoCalibrationResult(int calibratorId)
{
    StereoCalibrator* p_Calibrator = FindStereoCalibrator(m_Sinks, calibratorId);
    if (p_Calibrator == nullptr) {
        m_Logger->EnterLog("StereoCalibrator not found: " + std::to_string(calibratorId));
        return StereoCalibrationResult();
    }
    return p_Calibrator->GetCalibrationResult();
}

int Manager::CreateStereoDepthNode(StereoDepthBackendKind backend, StereoCalibrationResult calibration,
    double minDepthMeters, double maxDepthMeters, int maxSkewUs, StereoFrameOutput frameOutput)
{
    int id = GenerateUUID();
    return CreateStereoDepthNode(id, backend, calibration, minDepthMeters, maxDepthMeters, maxSkewUs, frameOutput);
}

int Manager::CreateStereoDepthNode(int id, StereoDepthBackendKind backend, StereoCalibrationResult calibration,
    double minDepthMeters, double maxDepthMeters, int maxSkewUs, StereoFrameOutput frameOutput)
{
    m_Logger->EnterLog("CreateStereoDepthNode called with id=" + std::to_string(id) + ", backend=" + std::to_string(backend));

    auto p_Node = std::make_shared<StereoDepthNode>(m_Logger, std::to_string(id), backend, calibration,
        minDepthMeters, maxDepthMeters, maxSkewUs, frameOutput);

    // dual-role: ISink (consumes the left/right pair) and ISource (produces depth JSON + a
    // colormap/rectified frame) - a downstream WebRTCSink or DepthFusionNode binds to this same id
    m_Sinks.emplace(id, p_Node);
    m_Sources.emplace(id, p_Node);

    m_Logger->EnterLog("StereoDepthNode created with id=" + std::to_string(id));
    return id;
}

string Manager::GetStereoDepthBackendName(int sinkId)
{
    StereoDepthNode* p_Node = FindStereoDepthNode(m_Sinks, sinkId);
    return p_Node == nullptr ? "" : p_Node->GetBackendName();
}

double Manager::GetStereoDepthValidFraction(int sinkId)
{
    StereoDepthNode* p_Node = FindStereoDepthNode(m_Sinks, sinkId);
    return p_Node == nullptr ? 0.0 : p_Node->GetLastValidFraction();
}

double Manager::GetStereoDepthMedianDepthMeters(int sinkId)
{
    StereoDepthNode* p_Node = FindStereoDepthNode(m_Sinks, sinkId);
    return p_Node == nullptr ? 0.0 : p_Node->GetLastMedianDepthMeters();
}

int Manager::CreateDepthFusionNode()
{
    int id = GenerateUUID();
    return CreateDepthFusionNode(id);
}

int Manager::CreateDepthFusionNode(int id)
{
    m_Logger->EnterLog("CreateDepthFusionNode called with id=" + std::to_string(id));

    auto p_Node = std::make_shared<DepthFusionNode>(m_Logger, std::to_string(id));

    // dual-role, same as every other detector-shaped node: ISink (consumes the detector's
    // bbox JSON) and ISource (produces the fused distance/bearing JSON + annotated frame)
    m_Sinks.emplace(id, p_Node);
    m_Sources.emplace(id, p_Node);

    m_Logger->EnterLog("DepthFusionNode created with id=" + std::to_string(id));
    return id;
}

bool Manager::SetDepthFusionDepthNode(int fusionSinkId, int depthNodeSourceId)
{
    auto sinkIt = m_Sinks.find(fusionSinkId);
    if (sinkIt == m_Sinks.end()) {
        m_Logger->EnterLog("SetDepthFusionDepthNode: fusion sink not found: " + std::to_string(fusionSinkId));
        return false;
    }
    DepthFusionNode* p_Fusion = dynamic_cast<DepthFusionNode*>(sinkIt->second.get());
    if (p_Fusion == nullptr) {
        m_Logger->EnterLog(LogLevel::Error, "SetDepthFusionDepthNode: sink " + std::to_string(fusionSinkId) + " is not a DepthFusionNode");
        return false;
    }

    auto sourceIt = m_Sources.find(depthNodeSourceId);
    if (sourceIt == m_Sources.end()) {
        m_Logger->EnterLog("SetDepthFusionDepthNode: depth node source not found: " + std::to_string(depthNodeSourceId));
        return false;
    }
    auto p_DepthNode = std::dynamic_pointer_cast<StereoDepthNode>(sourceIt->second);
    if (!p_DepthNode) {
        m_Logger->EnterLog(LogLevel::Error, "SetDepthFusionDepthNode: source " + std::to_string(depthNodeSourceId) + " is not a StereoDepthNode");
        return false;
    }

    p_Fusion->SetStereoDepthNode(p_DepthNode);
    return true;
}

int Manager::CreateObjectDetectionSink(ObjectDetectionProvider provider)
{
    throw std::runtime_error("CreateObjectDetectionSink requires a model - use the overload that takes modelPath/labelsPath/variant/thresholds");
}

int Manager::CreateObjectDetectionSink(ObjectDetectionProvider provider, int id)
{
    throw std::runtime_error("CreateObjectDetectionSink requires a model - use the overload that takes modelPath/labelsPath/variant/thresholds");
}

int Manager::CreateObjectDetectionSink(ObjectDetectionProvider provider, string modelPath, string labelsPath,
    YoloVariant variant, float confThreshold, float nmsThreshold, int inputSize)
{
    int id = GenerateUUID();
    return CreateObjectDetectionSink(id, provider, modelPath, labelsPath, variant, confThreshold, nmsThreshold, inputSize);
}

int Manager::CreateObjectDetectionSink(int id, ObjectDetectionProvider provider, string modelPath, string labelsPath,
    YoloVariant variant, float confThreshold, float nmsThreshold, int inputSize)
{
    m_Logger->EnterLog("CreateObjectDetectionSink called with id=" + std::to_string(id) + ", modelPath=" + modelPath);

    DetectionBackendConfig config;
    config.modelPath = modelPath;
    config.labelsPath = labelsPath;
    config.variant = variant;
    config.confThreshold = confThreshold;
    config.nmsThreshold = nmsThreshold;
    config.inputWidth = inputSize;
    config.inputHeight = inputSize;

    std::shared_ptr<IDetectionBackend> backend;
    switch (provider) {
#ifdef FRCV_WITH_ONNX
        case ONNX:
            backend = std::make_shared<OnnxDetectionBackend>();
            break;
#endif
        case RKNN:
            throw std::runtime_error("RKNN object detection backend is not implemented yet - use ONNX");
        default:
            throw std::runtime_error("unknown ObjectDetectionProvider");
    }

    if (!backend || !backend->Load(config)) {
        throw std::runtime_error("failed to load detection model: " + modelPath);
    }

    auto p_Sink = std::make_shared<ObjectDetectionSink>(m_Logger, std::to_string(id), backend);

    // ObjectDetectionSink is both an ISink (consumes camera frames) and an ISource (produces
    // detections), so it must be registered in both maps to be reachable from either side
    m_Sinks.emplace(id, p_Sink);
    m_Sources.emplace(id, p_Sink);

    m_Logger->EnterLog("ObjectDetectionSink created with id=" + std::to_string(id) + " using backend=" + backend->Name());
    return id;
}

int Manager::CreateRecordingSink(int sourceId)
{
    int id = GenerateUUID();

    // TODO: generate the path to the video file, an empty string will couse a faliure

    /*RecordSink* p_RecordSink = new RecordSink(m_Logger, "");

    m_Sinks.emplace(id, p_RecordSink);*/

    return id;
}

#ifdef FRCV_WITH_NT4
int Manager::CreateNetworkTablesSinkForTeam(int teamNumber, string rootTable, string clientIdentity)
{
    int id = GenerateUUID();
    return CreateNetworkTablesSinkForTeam(id, teamNumber, rootTable, clientIdentity);
}

int Manager::CreateNetworkTablesSinkForTeam(int id, int teamNumber, string rootTable, string clientIdentity)
{
    NetworkTablesConfig config;
    config.teamNumber = static_cast<unsigned int>(teamNumber);
    config.rootTable = rootTable;
    config.clientIdentity = clientIdentity;

    m_Logger->EnterLog("CreateNetworkTablesSinkForTeam called with id=" + std::to_string(id) + ", team=" + std::to_string(teamNumber));

    auto p_Sink = std::make_shared<NetworkTablesSink>(m_Logger, std::to_string(id), config);

    // terminal sink: it has no ISource half, so it only ever goes into m_Sinks
    m_Sinks.emplace(id, p_Sink);
    return id;
}

int Manager::CreateNetworkTablesSinkForServer(string serverAddress, int port, string rootTable, string clientIdentity)
{
    int id = GenerateUUID();
    return CreateNetworkTablesSinkForServer(id, serverAddress, port, rootTable, clientIdentity);
}

int Manager::CreateNetworkTablesSinkForServer(int id, string serverAddress, int port, string rootTable, string clientIdentity)
{
    NetworkTablesConfig config;
    config.serverAddress = serverAddress;
    config.port = static_cast<unsigned int>(port);
    config.rootTable = rootTable;
    config.clientIdentity = clientIdentity;

    m_Logger->EnterLog("CreateNetworkTablesSinkForServer called with id=" + std::to_string(id) + ", server=" + serverAddress);

    auto p_Sink = std::make_shared<NetworkTablesSink>(m_Logger, std::to_string(id), config);
    m_Sinks.emplace(id, p_Sink);
    return id;
}

bool Manager::IsNetworkTablesSinkConnected(int sinkId)
{
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) return false;

    NetworkTablesSink* p_NtSink = dynamic_cast<NetworkTablesSink*>(sink->second.get());
    return p_NtSink != nullptr && p_NtSink->IsConnected();
}

string Manager::GetNetworkTablesSinkStatus(int sinkId)
{
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) {
        m_Logger->EnterLog("Sink not found: " + std::to_string(sinkId));
        return "{}";
    }

    NetworkTablesSink* p_NtSink = dynamic_cast<NetworkTablesSink*>(sink->second.get());
    if (p_NtSink == nullptr) {
        m_Logger->EnterLog("Sink " + std::to_string(sinkId) + " is not a NetworkTablesSink");
        return "{}";
    }

    return p_NtSink->GetConnectionStatus();
}
#endif

#ifdef FRCV_WITH_WEBRTC
int Manager::CreateWebRTCSink(int bitrateKbps, int fps, string encoderName)
{
    int id = GenerateUUID();
    return CreateWebRTCSink(id, bitrateKbps, fps, encoderName);
}

int Manager::CreateWebRTCSink(int id, int bitrateKbps, int fps, string encoderName)
{
    WebRTCSinkConfig config;
    config.bitrateKbps = bitrateKbps;
    config.fps = fps;
    config.encoderName = encoderName;

    m_Logger->EnterLog("CreateWebRTCSink called with id=" + std::to_string(id) + ", encoder=" + encoderName);

    auto p_Sink = std::make_shared<WebRTCSink>(m_Logger, std::to_string(id), config);
    m_Sinks.emplace(id, p_Sink);
    return id;
}

namespace {
    // shared by all WebRTC accessor methods below - a WebRTCSink lookup + cast is used
    // repeatedly, and every failure mode should behave the same way
    std::shared_ptr<WebRTCSink> FindWebRTCSink(map<int, std::shared_ptr<ISink>>& sinks, int sinkId)
    {
        auto sink = sinks.find(sinkId);
        if (sink == sinks.end()) return nullptr;
        return std::dynamic_pointer_cast<WebRTCSink>(sink->second);
    }
}

string Manager::WebRTCCreateOffer(int sinkId)
{
    auto sink = FindWebRTCSink(m_Sinks, sinkId);
    if (!sink) throw std::runtime_error("no WebRTCSink with id " + std::to_string(sinkId));
    return sink->CreateOffer();
}

void Manager::WebRTCSetAnswer(int sinkId, string sdp)
{
    auto sink = FindWebRTCSink(m_Sinks, sinkId);
    if (!sink) throw std::runtime_error("no WebRTCSink with id " + std::to_string(sinkId));
    sink->SetAnswer(sdp);
}

void Manager::WebRTCAddIceCandidate(int sinkId, string candidate, string mid)
{
    auto sink = FindWebRTCSink(m_Sinks, sinkId);
    if (!sink) throw std::runtime_error("no WebRTCSink with id " + std::to_string(sinkId));
    sink->AddIceCandidate(candidate, mid);
}

bool Manager::IsWebRTCSinkConnected(int sinkId)
{
    auto sink = FindWebRTCSink(m_Sinks, sinkId);
    return sink != nullptr && sink->IsConnected();
}

string Manager::GetWebRTCSinkStatus(int sinkId)
{
    auto sink = FindWebRTCSink(m_Sinks, sinkId);
    return sink ? sink->GetConnectionStatus() : "{}";
}
#endif

void Manager::StartAllSources()
{
    auto iterator = m_Sources.begin();

    while (iterator != m_Sources.end()) {
        iterator->second->Toggle(true);
        iterator++;
    }
}

void Manager::StopAllSources()
{
    auto iterator = m_Sources.begin();

    while (iterator != m_Sources.end()) {
        iterator->second->Toggle(false);
        iterator++;
    }
}

bool Manager::StopSourceById(int sourceId)
{
    auto source = m_Sources.find(sourceId);
    if (source == m_Sources.end()) {
        return false;
    }
    source->second->Toggle(false);
    return true;
}

bool Manager::StartSourceById(int sourceId)
{
    auto source = m_Sources.find(sourceId);
    if (source == m_Sources.end()) {
        return false;
    }
    source->second->Toggle(true);
    return true;
}

bool Manager::IsSourceActive(int sourceId)
{
	// TODO: implement this function
	auto source = m_Sources.find(sourceId);
    if (source == m_Sources.end()) {
        return false;
    }
    return source->second->GetToggleStatus();
}

void Manager::StartAllSinks() {
    if (!m_Sinks.empty()) {
        auto iterator = m_Sinks.begin();

        while (iterator != m_Sinks.end()) {
            iterator->second->Toggle(true);
            iterator++;
        }
    }
}

void Manager::StopAllSinks() {
    auto iterator = m_Sinks.begin();

    while (iterator != m_Sinks.end()) {
        iterator->second->Toggle(false);
        iterator++;
    }
}

bool Manager::StopSinkById(int sinkId) {
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) {
        return false;
    }
    sink->second->Toggle(false);
    return true;
}

bool Manager::IsSinkActive(int sinkId)
{
	auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) {
		return false;
	}
    return sink->second->GetToggleStatus();
}

bool Manager::StartSinkById(int sinkId) {
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) {
        return false;
    }
    sink->second->Toggle(true);
    return true;
}

string Manager::GetSinkResult(int sinkId)
{
    m_Logger->EnterLog("GetSinkResult called with sinkId=" + std::to_string(sinkId));
    auto sink = m_Sinks.find(sinkId);
    if (sink == m_Sinks.end()) {
        m_Logger->EnterLog("Sink not found: " + std::to_string(sinkId));
        return "{}";
    }

    // a sink's result is only meaningful when it is also a source (ApriltagDetector,
    // CameraCalibrator, future ObjectDetectionSink); terminal sinks (RecordSink, and in future
    // NetworkTablesSink/WebRTCSink) consume results but don't produce any of their own
    ISource* p_AsSource = dynamic_cast<ISource*>(sink->second.get());
    if (p_AsSource == nullptr) {
        return "{}";
    }

    SourceResult result = p_AsSource->GetLatestResult();
    if (!result.json.has_value()) {
        return "{}";
    }

    return result.json.value().dump();
}

string Manager::GetAllSinkResults()
{
    m_Logger->EnterLog("GetAllSinkResults called");
    nlohmann::json allResults = nlohmann::json::object();

    for (auto& sinkPair : m_Sinks) {
        ISource* p_AsSource = dynamic_cast<ISource*>(sinkPair.second.get());
        if (p_AsSource == nullptr) {
            continue;
        }

        SourceResult result = p_AsSource->GetLatestResult();
        if (result.json.has_value()) {
            allResults[std::to_string(sinkPair.first)] = result.json.value();
        }
    }

    return allResults.dump();
}

//bool Manager::SetSinkResult(int sinkId, string result)
//{
//    m_Logger->EnterLog("SetSinkResult called with sinkId=" + std::to_string(sinkId) + ", result=" + result);
//    if (m_Sinks.find(sinkId) == m_Sinks.end()) {
//        m_Logger->EnterLog("Result entry not found for sinkId: " + std::to_string(sinkId));
//        return false;
//    }
//    else {
//        // TODO: fix
//        m_Sinks.find(sinkId)->second->() = result;
//    }
//        m_Logger->EnterLog("Result set for sinkId: " + std::to_string(sinkId));
//    return true;
//}

int Manager::GenerateUUID()
{
    m_Logger->EnterLog("GenerateUUID called");
    std::mt19937 engine(std::chrono::high_resolution_clock::now().time_since_epoch().count());

    std::uniform_int_distribution<int> dist(0, 2147483647);

    int randomNumber = dist(engine);
    m_Logger->EnterLog("Generated UUID: " + std::to_string(randomNumber));
    return randomNumber;
}

//int Manager::CreateCameraCalibrationSink(int width, int height)
//{
//	int id = GenerateUUID();
//
//    // TODO: fix
//    auto p_Sink = std::make_shared<CameraCalibrationSink>(m_Logger, nullptr, FrameSpec(height, width, CV_8UC3));
//
//	m_CameraCalibrationSinks.emplace(id, p_Sink);
//
//
//    return id;
//}

//void Manager::BindSourceToCalibrationSink(int sourceId)
//{
//	auto sink = m_CameraCalibrationSinks.find(sourceId);
//    if (sink != m_CameraCalibrationSinks.end() && m_Sources.find(sourceId) != m_Sources.end()) {
//        sink->second->BindSource(m_Sources.find(sourceId)->second.get());
//    }
//}
//
//void Manager::CameraCalibrationSinkGrabFrame(int sinkId)
//{
//    auto sink = m_CameraCalibrationSinks.find(sinkId);
//    if (sink != m_CameraCalibrationSinks.end()) {
//        sink->second->GrabAndProcessFrame();
//    } else {
//        m_Logger->EnterLog("CameraCalibrationSink not found with id: " + std::to_string(sinkId));
//	}
//}
//
//CameraCalibrationResult Manager::GetCameraCalibrationResults(int sinkId)
//{
//	auto sink = m_CameraCalibrationSinks.find(sinkId);
//    if (sink != m_CameraCalibrationSinks.end()) {
//		return sink->second->GetResults();
//    }
//    // Return empty result if not found
//    return CameraCalibrationResult();
//}

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

