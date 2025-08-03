#pragma once

#include <apriltag/apriltag.h>
#include "ISink.h"
#include "ApriltagDetection.h"
#include <vector>
#include "Logger.h"
#include <string>

class FramePool;
class PreProcessor;
class CameraCalibrationResult;
// Forward declare Logger to avoid header conflicts

class ApriltagSink : public ISink
{
public:
	ApriltagSink(Logger* logger, PreProcessor* preProcessor, FramePool* framePool);
    ~ApriltagSink();
	void addCameraInfo(CameraCalibrationResult cameraInfo);
	std::string GetStatus() override;
	void ProcessFrame() override;
	void CreatePreview() override;
private:
	PreProcessor* preProcessor;
	apriltag_family_t* family;
	apriltag_detector_t* detector;
	apriltag_detection_info_t info;
	
	// stuff for preview
	std::vector<ApriltagDetection> m_PreviewData;
	FramePool* m_FramePool;
};
