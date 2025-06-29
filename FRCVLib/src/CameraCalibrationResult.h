#pragma once

class CameraCalibrationResult {
public:
    // Camera intrinsic parameters
    double fx; // Focal length in x (pixels)
    double fy; // Focal length in y (pixels)
    double cx; // Principal point x (pixels)
    double cy; // Principal point y (pixels)

    CameraCalibrationResult()
        : fx(0.0), fy(0.0), cx(0.0), cy(0.0) {
    }

    CameraCalibrationResult(double fx, double fy, double cx, double cy)
        : fx(fx), fy(fy), cx(cx), cy(cy) {
    }
};

