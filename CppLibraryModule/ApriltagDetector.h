//
// Created by yojobama on 5/24/25.
//

#ifndef APRILTAGDETECTOR_H
#define APRILTAGDETECTOR_H

#include <apriltag.h>
#include <tag36h11.h>
#include <vector>
#include <opencv2/core/mat.hpp>

#include "apriltag_pose.h"

struct ApriltagDetection {
    apriltag_detection_t detection;
    apriltag_pose_t position;
    double err;

    ApriltagDetection() = default;

    ApriltagDetection(apriltag_detection_t raw_det, apriltag_pose_t pose, double err) {
        detection = raw_det;
        position = pose;
        this->err = err;
    }
};

struct CameraCalibrationInfo {
    double fx, fy, cx, cy;
};

class ApriltagDetector {
public:
    ApriltagDetector(CameraCalibrationInfo cameraCalibrationInfo, double tagSize);
    ~ApriltagDetector();
    void detect(cv::Mat &image, std::vector<ApriltagDetection> &detections);
private:
    apriltag_family_t *family;
    apriltag_detector_t *detector;
    CameraCalibrationInfo cameraCalibrationInfo{};
    double tagSize;
};



#endif //APRILTAGDETECTOR_H
