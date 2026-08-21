%module libFRCVLib
%{
#include "Manager.h"
#include "CameraCalibrationResult.h"
#include "IDetectionBackend.h"
%}

%include "std_string.i"
%include "std_vector.i"
%include "std_unique_ptr.i"

// IDetectionBackend.h must be %include'd explicitly (not just #include'd from within Manager.h)
// for SWIG to see YoloVariant as a real enum rather than falling back to an opaque
// SWIGTYPE_p_YoloVariant handle - confirmed by testing: relying on Manager.h's own #include of
// it was not enough, even though SWIG's preprocessor otherwise follows #include chains fine
// (e.g. through ISink.h -> SourceResult.h -> <opencv2/opencv.hpp>).
%include "IDetectionBackend.h"
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
