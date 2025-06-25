%module FRCVCore
%{
#include "Manager.h"
#include "CameraCalibrationResult.h"
#include "Logger.h"
%}

%include "Manager.h"
%include "CameraCalibrationResult.h"
%include "Logger.h"

%include "std_string.i"
%include "std_vector.i"

namespace std {
    %template(VectorInt) vector<int>;
    %template(VectorString) vector<string>;
    %template(VectorLog) vector<Log>;
    %template(VectorCameraHardwareInfo) vector<CameraHardwareInfo>;
}