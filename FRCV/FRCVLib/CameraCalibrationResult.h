#pragma once

#include <apriltag/apriltag.h>
#include "FrameSpec.h"

class CameraCalibrationResult {
public:
    // Camera intrinsic parameters
    double fx; // Focal length in x (pixels)
    double fy; // Focal length in y (pixels)
    double cx; // Principal point x (pixels)
    double cy; // Principal point y (pixels)
	double rms; // Root Mean Square error of the calibration, <0.5 is good, <1.0 is acceptable, >1.0 is bad
    FrameSpec frameSpec;

    CameraCalibrationResult()
        : fx(0.0), fy(0.0), cx(0.0), cy(0.0), frameSpec() {
    }

    CameraCalibrationResult(double fx, double fy, double cx, double cy, double rms)
        : fx(fx), fy(fy), cx(cx), cy(cy), rms(rms), frameSpec() {
    }
};

