#include "ApriltagSink.h"
#include <vector>
#include <apriltag/tag36h11.h>
#include "Frame.h"
#include "CameraCalibrationResult.h"
#include "PreProcessor.h"
#include <opencv2/opencv.hpp>
#include "FramePool.h"

ApriltagSink::ApriltagSink(Logger* logger, PreProcessor* preProcessor, FramePool* framePool) : ISink(logger) {
    if (logger) logger->EnterLog("ApriltagSink constructed");
	this->family = tag36h11_create();
	this->detector = apriltag_detector_create();
	apriltag_detector_add_family(this->detector, this->family);
	this->preProcessor = preProcessor;
	m_FramePool = framePool;
}

ApriltagSink::~ApriltagSink() {
    if (m_Logger) m_Logger->EnterLog("ApriltagSink destructed");
    // Clean up resources
    if (this->detector) {
        apriltag_detector_destroy(this->detector);
    }
    if (this->family) {
        tag36h11_destroy(this->family);
    }
}

void ApriltagSink::addCameraInfo(CameraCalibrationResult cameraInfo)
{
	info.cx = cameraInfo.cx;
	info.cy = cameraInfo.cy;
	info.fx = cameraInfo.fx;
	info.fy = cameraInfo.fy;
}

std::string ApriltagSink::GetStatus()
{
    // Pseudocode:
    // 1. Log entry if logger is available.
    // 2. Gather status information: detector/family pointers, source binding, etc.
    // 3. Format as JSON string.
    // 4. Return the JSON string.

    if (m_Logger) m_Logger->EnterLog("ApriltagSink::getStatus called");

    bool detectorReady = (detector != nullptr);
    bool familyReady = (family != nullptr);
    bool sourceBound = (m_Source != nullptr);

    std::string status = "{";
    status += "\"detectorReady\":" + std::string(detectorReady ? "true" : "false") + ",";
    status += "\"familyReady\":" + std::string(familyReady ? "true" : "false") + ",";
    status += "\"sourceBound\":" + std::string(sourceBound ? "true" : "false");
    status += "}";

    return status;
}

void ApriltagSink::ProcessFrame()
{
	if (m_Logger) m_Logger->EnterLog("ApriltagSink::getResults called");
	std::shared_ptr<Frame> frame = m_Source->GetLatestFrame();
	
	std::string returnString = "{\"detections\":[";
	
	if (frame != nullptr) {
		FrameSpec spec = frame->GetSpec();

		spec.SetType(CV_8UC1);
		//if (logger) logger->enterLog("spec.type after setType: " + std::to_string(spec.getType()));

		std::shared_ptr<Frame> gray = preProcessor->transformFrame(frame, spec);

		std::vector<ApriltagDetection> returnVector;

		m_Logger->EnterLog("making an image_u8_t from the opencv frame");

		image_u8_t img = {
			gray->cols,
			gray->rows,
			gray->cols,
			gray->data
		};

		m_Logger->EnterLog("detecting apriltags using the detector");
		zarray_t* detections = apriltag_detector_detect(detector, &img);

		for (int i = 0; i < zarray_size(detections); i++) {
			apriltag_detection_t* detection;
			zarray_get(detections, i, &detection);

			apriltag_pose_t pose;
			/*double err = estimate_tag_pose(&info, &pose);*/

			returnVector.push_back(ApriltagDetection(*detection, pose));
		}

		// save the results for preview creation if preview is enabled
		if (GetPreviewStatus()) {
			m_PreviewData = returnVector;
		}

		for (size_t i = 0; i < returnVector.size(); ++i) {
			returnString += returnVector[i].ToString();
			if (i != returnVector.size() - 1) {
				returnString += ",";
			}
		}
	}
	else {
		m_Logger->EnterLog(LogLevel::Error, "ApriltagSink::getResults: Frame is null");
		returnString += "{}";
	}

	returnString += "]}";

	m_Results = returnString;
}

void ApriltagSink::CreatePreview()
{
    if (!GetPreviewStatus() || m_Source == nullptr) return;

    std::shared_ptr<Frame> latestFrame = m_Source->GetLatestFrame();
    if (!latestFrame) return;

    // Create color copy for visualization
    FrameSpec colorSpec = latestFrame->GetSpec();
    colorSpec.SetType(CV_8UC3);
    std::shared_ptr<Frame> colorFrame = m_FramePool->GetFrame(colorSpec);
    cv::cvtColor(*latestFrame, *colorFrame, cv::COLOR_GRAY2BGR);

    // Colors for visualization
    const cv::Scalar RED(0, 0, 255);
    const cv::Scalar GREEN(0, 255, 0);
    const cv::Scalar BLUE(255, 0, 0);
    
    for (const auto& detection : m_PreviewData) {
        // Draw tag outline
        for (int i = 0; i < 4; i++) {
            cv::Point p1(detection.m_pDetection->p[i][0], detection.m_pDetection->p[i][1]);
            cv::Point p2(detection.m_pDetection->p[(i+1)%4][0], detection.m_pDetection->p[(i+1)%4][1]);
            cv::line(*colorFrame, p1, p2, GREEN, 2);
        }

        // Draw tag center and ID
        cv::Point center(detection.m_pDetection->c[0], detection.m_pDetection->c[1]);
        cv::circle(*colorFrame, center, 3, RED, 2);
        cv::putText(*colorFrame, 
                    std::to_string(detection.m_pDetection->id),
                    cv::Point(center.x + 10, center.y),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.8,
                    BLUE,
                    2);

        // Draw tag corners
        for (int i = 0; i < 4; i++) {
            cv::Point corner(detection.m_pDetection->p[i][0], detection.m_pDetection->p[i][1]);
            cv::circle(*colorFrame, corner, 4, RED, -1);
        }
    }

    m_PreviewFrame = colorFrame;
}
