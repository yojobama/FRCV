%module libFRCVLib
%{
#include "Manager.h"
#include "CameraCalibrationResult.h"
#include "IDetectionBackend.h"
#include "IApriltagBackend.h"
#include "CalibrationBoardType.h"
%}

%include "std_string.i"
%include "std_vector.i"
%include "std_unique_ptr.i"

// Every plain-enum header Manager.h merely #includes must ALSO be %include'd explicitly here
// for SWIG to see it as a real enum rather than falling back to an opaque SWIGTYPE_p_* handle -
// confirmed three times now (YoloVariant, ApriltagBackendKind, CalibrationBoardType): relying
// on Manager.h's own #include is not enough, even though SWIG's preprocessor otherwise follows
// #include chains fine (e.g. through ISink.h -> SourceResult.h -> <opencv2/opencv.hpp>).
%include "IDetectionBackend.h"
%include "IApriltagBackend.h"
%include "CalibrationBoardType.h"
%include "Manager.h"
%include "CameraCalibrationResult.h"
%include "Logger.h"

namespace std {
    %template(VectorInt) vector<int>;
    %template(VectorDouble) vector<double>;
    %template(VectorString) vector<string>;
    %template(VectorLog) vector<Log>;
    %template(VectorCameraHardwareInfo) vector<CameraHardwareInfo>;
    %template(UniquePtrLog) unique_ptr<Log>;
}
