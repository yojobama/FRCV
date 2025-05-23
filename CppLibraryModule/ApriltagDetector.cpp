//
// Created by yojobama on 5/24/25.
//

#include "ApriltagDetector.h"

ApriltagDetector::ApriltagDetector(CameraCalibrationInfo cameraCalibrationInfo, double tagSize) {
    this->cameraCalibrationInfo = cameraCalibrationInfo;
    detector = apriltag_detector_create();
    family = tag36h11_create();
    apriltag_detector_add_family(detector, family);
    this.tagSize = tagSize;
}

ApriltagDetector::~ApriltagDetector() {
    apriltag_detector_destroy(detector);
    tag36h11_destroy(family);
}

void ApriltagDetector::detect(cv::Mat &image, std::vector<ApriltagDetection> &detections, bool poseEstimate) {
    image_u8_t im = {
        .height = image.rows,
        .stride = image.cols,
        .buf = image.data
    };

    zarray_t* rawDetections = apriltag_detector_detect(detector, &im);

    for (int i = 0; i < zarray_size(rawDetections); i++) {
        apriltag_detection_t *rawDet;
        zarray_get(rawDetections, i, &rawDet);

        if (poseEstimate) {
            apriltag_detection_info_t info;
            info.det = rawDet;
            info.tagsize = tagSize;
            info.fx = cameraCalibrationInfo.fx;
            info.fy = cameraCalibrationInfo.fy;
            info.cx = cameraCalibrationInfo.cx;
            info.cy = cameraCalibrationInfo.cy;

            apriltag_pose_t pose;
            double err = estimate_tag_pose(&info, &pose);

            detections.push_back(ApriltagDetection(*rawDet, pose, err));
        } else {
            detections.push_back(ApriltagDetection(*rawDet, nullptr, nullptr));
        }
    }
}
