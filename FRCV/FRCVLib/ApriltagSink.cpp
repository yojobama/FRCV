#include "ApriltagSink.h"

#include <apriltag/tag36h11.h>

ApriltagSink::ApriltagSink(Logger* logger) {
	this->logger = logger;
	this->family = tag36h11_create();
	this->detector = apriltag_detector_create();
	apriltag_detector_add_family(this->detector, this->family);
}

std::vector<ApriltagDetection> ApriltagSink::getDetections(Frame* frame) {
	
	std::vector<ApriltagDetection> returnVector;
	
	logger->enterLog(DEBUG, "making an image_u8_t from the opencv frame");
	image_u8_t img = {
		.width = frame->cols,
		.height = frame->rows,
		.stride = frame->cols,
		.buf = frame->data
	};

	logger->enterLog(DEBUG, "detecting apriltags using the detector");
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