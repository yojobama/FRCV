#include "ApriltagSink.h"
#include <vector>
#include <apriltag/tag36h11.h>
#include "Frame.h"
#include "CameraCalibrationResult.h"
#include "PreProcessor.h"
#include <opencv2/opencv.hpp>

ApriltagSink::ApriltagSink(Logger* logger, PreProcessor* preProcessor) : ISink(logger), logger(logger) {
    if (logger) logger->enterLog("ApriltagSink constructed");
	this->family = tag36h11_create();
	this->detector = apriltag_detector_create();
	apriltag_detector_add_family(this->detector, this->family);
	this->preProcessor = preProcessor;
}

ApriltagSink::~ApriltagSink() {
    if (logger) logger->enterLog("ApriltagSink destructed");
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

std::string ApriltagSink::getStatus()
{
    // Pseudocode:
    // 1. Log entry if logger is available.
    // 2. Gather status information: detector/family pointers, source binding, etc.
    // 3. Format as JSON string.
    // 4. Return the JSON string.

    if (logger) logger->enterLog("ApriltagSink::getStatus called");

    bool detectorReady = (detector != nullptr);
    bool familyReady = (family != nullptr);
    bool sourceBound = (source != nullptr);

    std::string status = "{";
    status += "\"detectorReady\":" + std::string(detectorReady ? "true" : "false") + ",";
    status += "\"familyReady\":" + std::string(familyReady ? "true" : "false") + ",";
    status += "\"sourceBound\":" + std::string(sourceBound ? "true" : "false");
    status += "}";

    return status;
}

void ApriltagSink::processFrame()
{
	if (logger) logger->enterLog("ApriltagSink::getResults called");
	Frame* frame = source->getLatestFrame();
	
	std::string returnString = "{\"detections\":[";
	
	if (frame != nullptr) {
		FrameSpec spec = frame->getSpec();

		spec.setType(CV_8UC1);
		//if (logger) logger->enterLog("spec.type after setType: " + std::to_string(spec.getType()));

		Frame* gray = preProcessor->transformFrame(frame, spec);

		std::vector<ApriltagDetection> returnVector;

		logger->enterLog("making an image_u8_t from the opencv frame");

		image_u8_t img = {
			gray->cols,
			gray->rows,
			gray->cols,
			gray->data
		};

		logger->enterLog("detecting apriltags using the detector");
		zarray_t* detections = apriltag_detector_detect(detector, &img);

		for (int i = 0; i < zarray_size(detections); i++) {
			apriltag_detection_t* detection;
			zarray_get(detections, i, &detection);

			apriltag_pose_t pose;
			/*double err = estimate_tag_pose(&info, &pose);*/

			returnVector.push_back(ApriltagDetection(*detection, pose));
		}

		for (size_t i = 0; i < returnVector.size(); ++i) {
			returnString += returnVector[i].toString();
			if (i != returnVector.size() - 1) {
				returnString += ",";
			}
		}
		frame->dereference();
	}

	returnString += "]}";

	results = returnString;
}
