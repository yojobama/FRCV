%module libFRCVLib
%{
#include "Manager.h"
#include "CameraCalibrationResult.h"
%}

%include "std_string.i"
%include "std_vector.i"
%include "std_unique_ptr.i"

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
