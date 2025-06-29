//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (https://www.swig.org).
// Version 4.3.0
//
// Do not make changes to this file unless you know what you are doing - modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class Manager : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal Manager(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(Manager obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  internal static global::System.Runtime.InteropServices.HandleRef swigRelease(Manager obj) {
    if (obj != null) {
      if (!obj.swigCMemOwn)
        throw new global::System.ApplicationException("Cannot release ownership as memory is not owned");
      global::System.Runtime.InteropServices.HandleRef ptr = obj.swigCPtr;
      obj.swigCMemOwn = false;
      obj.Dispose();
      return ptr;
    } else {
      return new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
    }
  }

  ~Manager() {
    Dispose(false);
  }

  public void Dispose() {
    Dispose(true);
    global::System.GC.SuppressFinalize(this);
  }

  protected virtual void Dispose(bool disposing) {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          FRCVCorePINVOKE.delete_Manager(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
    }
  }

  public Manager() : this(FRCVCorePINVOKE.new_Manager(), true) {
  }

  public VectorInt getAllSinks() {
    VectorInt ret = new VectorInt(FRCVCorePINVOKE.Manager_getAllSinks(swigCPtr), true);
    return ret;
  }

  public VectorCameraHardwareInfo enumerateAvailableCameras() {
    VectorCameraHardwareInfo ret = new VectorCameraHardwareInfo(FRCVCorePINVOKE.Manager_enumerateAvailableCameras(swigCPtr), true);
    return ret;
  }

  public bool bindSourceToSink(int sourceId, int sinkId) {
    bool ret = FRCVCorePINVOKE.Manager_bindSourceToSink(swigCPtr, sourceId, sinkId);
    return ret;
  }

  public bool unbindSourceFromSink(int sinkId) {
    bool ret = FRCVCorePINVOKE.Manager_unbindSourceFromSink(swigCPtr, sinkId);
    return ret;
  }

  public int createCameraSource(CameraHardwareInfo info) {
    int ret = FRCVCorePINVOKE.Manager_createCameraSource(swigCPtr, CameraHardwareInfo.getCPtr(info));
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public int createVideoFileSource(SWIGTYPE_p_std__string path, int fps) {
    int ret = FRCVCorePINVOKE.Manager_createVideoFileSource(swigCPtr, SWIGTYPE_p_std__string.getCPtr(path), fps);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public int createImageFileSource(SWIGTYPE_p_std__string path) {
    int ret = FRCVCorePINVOKE.Manager_createImageFileSource(swigCPtr, SWIGTYPE_p_std__string.getCPtr(path));
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public int createApriltagSink() {
    int ret = FRCVCorePINVOKE.Manager_createApriltagSink(swigCPtr);
    return ret;
  }

  public int createObjectDetectionSink() {
    int ret = FRCVCorePINVOKE.Manager_createObjectDetectionSink(swigCPtr);
    return ret;
  }

  public int createRecordingSink(int sourceId) {
    int ret = FRCVCorePINVOKE.Manager_createRecordingSink(swigCPtr, sourceId);
    return ret;
  }

  public void startAllSources() {
    FRCVCorePINVOKE.Manager_startAllSources(swigCPtr);
  }

  public void stopAllSources() {
    FRCVCorePINVOKE.Manager_stopAllSources(swigCPtr);
  }

  public bool stopSourceById(int sourceId) {
    bool ret = FRCVCorePINVOKE.Manager_stopSourceById(swigCPtr, sourceId);
    return ret;
  }

  public bool startSourceById(int sourceId) {
    bool ret = FRCVCorePINVOKE.Manager_startSourceById(swigCPtr, sourceId);
    return ret;
  }

  public void startAllSinks() {
    FRCVCorePINVOKE.Manager_startAllSinks(swigCPtr);
  }

  public void stopAllSinks() {
    FRCVCorePINVOKE.Manager_stopAllSinks(swigCPtr);
  }

  public bool startSinkById(int sinkId) {
    bool ret = FRCVCorePINVOKE.Manager_startSinkById(swigCPtr, sinkId);
    return ret;
  }

  public bool stopSinkById(int sinkId) {
    bool ret = FRCVCorePINVOKE.Manager_stopSinkById(swigCPtr, sinkId);
    return ret;
  }

  public SWIGTYPE_p_std__string getAllSinkStatus() {
    SWIGTYPE_p_std__string ret = new SWIGTYPE_p_std__string(FRCVCorePINVOKE.Manager_getAllSinkStatus(swigCPtr), true);
    return ret;
  }

  public SWIGTYPE_p_std__string getSinkStatusById(int sinkId) {
    SWIGTYPE_p_std__string ret = new SWIGTYPE_p_std__string(FRCVCorePINVOKE.Manager_getSinkStatusById(swigCPtr, sinkId), true);
    return ret;
  }

  public SWIGTYPE_p_std__string getSinkResult(int sinkId) {
    SWIGTYPE_p_std__string ret = new SWIGTYPE_p_std__string(FRCVCorePINVOKE.Manager_getSinkResult(swigCPtr, sinkId), true);
    return ret;
  }

  public SWIGTYPE_p_std__string getAllSinkResults() {
    SWIGTYPE_p_std__string ret = new SWIGTYPE_p_std__string(FRCVCorePINVOKE.Manager_getAllSinkResults(swigCPtr), true);
    return ret;
  }

  public SWIGTYPE_p_std__vectorT_Log_p_t getAllLogs() {
    global::System.IntPtr cPtr = FRCVCorePINVOKE.Manager_getAllLogs(swigCPtr);
    SWIGTYPE_p_std__vectorT_Log_p_t ret = (cPtr == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_std__vectorT_Log_p_t(cPtr, false);
    return ret;
  }

  public void clearAllLogs() {
    FRCVCorePINVOKE.Manager_clearAllLogs(swigCPtr);
  }

  public bool takeCalibrationImage(int cameraId) {
    bool ret = FRCVCorePINVOKE.Manager_takeCalibrationImage(swigCPtr, cameraId);
    return ret;
  }

  public CameraCalibrationResult conculdeCalibration() {
    CameraCalibrationResult ret = new CameraCalibrationResult(FRCVCorePINVOKE.Manager_conculdeCalibration(swigCPtr), true);
    return ret;
  }

}
