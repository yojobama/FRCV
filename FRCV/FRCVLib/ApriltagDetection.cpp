#include "ApriltagDetection.h"

ApriltagDetection::ApriltagDetection(apriltag_detection_t detection, apriltag_pose_t pose)
    : detection(new apriltag_detection_t(detection)), pose(pose), family(detection.family) {}
