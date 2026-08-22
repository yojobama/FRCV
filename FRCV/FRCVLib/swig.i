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
%include "exception.i"

// Without this, ANY C++ exception thrown by application code (a bad model file, a missing
// sink id, an unimplemented backend, ...) crosses the P/Invoke boundary completely uncaught and
// calls std::terminate(), killing the entire server process - not just the one request that
// triggered it. Confirmed three separate times this session (WebRTCSink::SetAnswer,
// WebRTCSink::AddIceCandidate, and Manager::CreateObjectDetectionSink on an invalid model file -
// each is a completely ordinary, expected failure mode, not a bug, yet each took the whole
// process down). SWIG's C# exception marshaling machinery (SWIGExceptionHelper /
// SWIGRegisterExceptionCallbacks, visible in every generated stack trace) already exists in the
// generated wrapper, but does nothing unless a wrapped call actually goes through SWIG_exception
// - which requires a %exception block. There were none anywhere in this file. This one applies
// globally to every function/method %include'd below, converting any std::exception (or truly
// unknown exception) into a real, catchable System.ApplicationException on the C# side instead.
%exception {
    try {
        $action
    } catch (const std::exception& e) {
        SWIG_exception(SWIG_RuntimeError, e.what());
    } catch (...) {
        SWIG_exception(SWIG_UnknownError, "unknown C++ exception");
    }
}

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
