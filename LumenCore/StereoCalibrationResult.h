#pragma once

#include "CameraCalibrationResult.h"
#include <vector>

// SWIG-safe, same discipline as CameraCalibrationResult.h: plain doubles and vector<double>, no
// cv:: types. StereoCalibrator.h (which produces this) pulls in
// <opencv2/objdetect/charuco_detector.hpp> and must never reach swig.i, but this result type is
// exactly the kind of thing the WebUI/NT4/downstream nodes need in hand, so it lives in its own
// header the way CameraCalibrationResult already does.
class StereoCalibrationResult {
public:
	// per-eye intrinsics + distortion, from cv::stereoCalibrate (or two prior CameraCalibrator
	// runs, if CALIB_FIX_INTRINSIC was used - see StereoCalibrator::RunCalibration)
	CameraCalibrationResult left;
	CameraCalibrationResult right;

	// right-relative-to-left extrinsics: R is row-major 3x3 (9 entries), T is 3x1 (3 entries)
	std::vector<double> R;
	std::vector<double> T;
	// essential / fundamental matrices, row-major 3x3 (9 entries each)
	std::vector<double> E;
	std::vector<double> F;

	// cv::stereoRectify output. R1/R2 are row-major 3x3 (9 entries each, rectifying rotations).
	// P1/P2 are row-major 3x4 (12 entries each, rectified projection matrices). Q is row-major
	// 4x4 (16 entries, disparity-to-depth reprojection matrix).
	std::vector<double> R1;
	std::vector<double> R2;
	std::vector<double> P1;
	std::vector<double> P2;
	std::vector<double> Q;

	double stereoRms = 0.0;       // cv::stereoCalibrate's own return value
	// mean |y_left - y_right| over the saved corner sets, pushed through the rectification maps -
	// the number that actually predicts codec-stereo density/validity (it gates blocks on |dy|),
	// NOT stereoRms. Gate real use at < 0.5px; see STEREO_IMPLEMENTATION_PLAN.md ss10.2.
	double epipolarRms = 0.0;
	double baselineMeters = 0.0;  // norm(T)
	double rectifiedFx = 0.0;     // P1[0] - the focal length codec-stereo's disparity->depth math needs
	double rectifiedCx = 0.0;
	double rectifiedCy = 0.0;
	int imageWidth = 0;
	int imageHeight = 0;

	// cv::stereoRectify's validPixROI1/2 - the sub-rectangle of the (alpha=0, already-cropped-
	// to-valid) rectified image that's actually guaranteed pixel-valid. StereoDepthNode crops to
	// the intersection of these (then rounds down to its backend's block size) rather than the
	// full rectified frame, so a motion-vector backend never gets to match against a black or
	// otherwise meaningless border - see STEREO_IMPLEMENTATION_PLAN.md ss10.3.
	int roiLeftX = 0, roiLeftY = 0, roiLeftW = 0, roiLeftH = 0;
	int roiRightX = 0, roiRightY = 0, roiRightW = 0, roiRightH = 0;

	StereoCalibrationResult() = default;

	bool IsValid() const { return !Q.empty() && baselineMeters > 0.0; }
};
