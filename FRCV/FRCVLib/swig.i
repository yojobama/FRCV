%module FRCVCore
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
    %template(VectorString) vector<string>;
    %template(VectorLog) vector<Log>;
    %template(VectorCameraHardwareInfo) vector<CameraHardwareInfo>;
    %template(VectorFrame) vector<Frame>;
    %template(VectorFrameSpec) vector<FrameSpec>;
    %template(UniquePtrLog) unique_ptr<Log>;
    %template(UniquePtrFrame) unique_ptr<Frame>;
}