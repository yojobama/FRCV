#include "ApriltagSink.h"
#include <vector>
#include <apriltag/tag36h11.h>

ApriltagSink::ApriltagSink(Logger* logger) : ISink<ApriltagDetection>(logger) {
	this->logger = logger;
	this->family = tag36h11_create();
	this->detector = apriltag_detector_create();
	apriltag_detector_add_family(this->detector, this->family);
}

ApriltagSink::~ApriltagSink() {
    // Clean up resources
    if (this->detector) {
        apriltag_detector_destroy(this->detector);
    }
    if (this->family) {
        tag36h11_destroy(this->family);
    }
}

std::vector<ApriltagDetection> ApriltagSink::getDetections(Frame* frame) {
	std::vector<ApriltagDetection> returnVector;

	logger->enterLog("making an image_u8_t from the opencv frame");

	image_u8_t img = {
		frame->cols,
		frame->rows,
		frame->cols,
		frame->data
	};

	logger->enterLog("detecting apriltags using the detector");
	zarray_t* detections = apriltag_detector_detect(detector, &img);

	for (int i = 0; i < zarray_size(detections); i++) {
		apriltag_detection_t* detection;
		zarray_get(detections, i, &detection);

		apriltag_pose_t pose;
		double err = estimate_tag_pose(&info, &pose);

		returnVector.push_back(ApriltagDetection(*detection, pose));
	}

	return returnVector;
}