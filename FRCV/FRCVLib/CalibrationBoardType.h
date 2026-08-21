#pragma once

// Split out from CameraCalibrator.h so Manager.h can expose a board type selector through
// SWIG without pulling in <opencv2/objdetect/charuco_detector.hpp> - a much heavier header
// than SWIG has been fed so far in this project, and untested against it.
//
// Plain (unscoped) enum for the same reason as YoloVariant/ApriltagBackendKind/
// CalibrationBoardType's other enum siblings in this codebase: SWIG wraps a plain C++ enum as
// a real C# enum but falls back to a broken opaque SWIGTYPE_p_* handle for a scoped one.
enum CalibrationBoardType {
	BOARD_CHECKERBOARD,
	BOARD_CHARUCO
};
