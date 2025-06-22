#include "ApriltagDetection.h"

ApriltagDetection::ApriltagDetection(apriltag_detection_t detection, apriltag_pose_t pose)
    : detection(new apriltag_detection_t(detection)), pose(pose), family(detection.family) {}

std::string ApriltagDetection::toString()
{
    std::ostringstream oss;
    oss << "{";

    // Detection parameters
    oss << "\"detection\":{";
    oss << "\"family\":\"" << (family ? family->name : "null") << "\",";
    oss << "\"id\":" << (detection ? detection->id : -1) << ",";
    oss << "\"hamming\":" << (detection ? detection->hamming : -1) << ",";
    oss << "\"decision_margin\":" << (detection ? detection->decision_margin : 0.0) << ",";
    oss << "\"center\":[";
    if (detection) {
        oss << detection->c[0] << "," << detection->c[1];
    } else {
        oss << "0,0";
    }
    oss << "],";
    oss << "\"corners\":[";
    if (detection) {
        for (int i = 0; i < 4; ++i) {
            oss << "[" << detection->p[i][0] << "," << detection->p[i][1] << "]";
            if (i < 3) oss << ",";
        }
    }
    oss << "]";
    oss << "},";

    // Pose parameters
    oss << "\"pose\":{";
    // Rotation matrix
    oss << "\"R\":[";
    if (pose.R && pose.R->data && pose.R->nrows == 3 && pose.R->ncols == 3) {
        for (unsigned int i = 0; i < 3; ++i) {
            oss << "[";
            for (unsigned int j = 0; j < 3; ++j) {
                oss << pose.R->data[i * 3 + j];
                if (j < 2) oss << ",";
            }
            oss << "]";
            if (i < 2) oss << ",";
        }
    }
    oss << "],";
    // Translation vector
    oss << "\"t\":[";
    if (pose.t && pose.t->data && pose.t->nrows == 3 && pose.t->ncols == 1) {
        for (unsigned int i = 0; i < 3; ++i) {
            oss << pose.t->data[i];
            if (i < 2) oss << ",";
        }
    }
    oss << "]";
    oss << "}";

    oss << "}";

    return oss.str();
}
