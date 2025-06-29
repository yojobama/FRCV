//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (https://www.swig.org).
// Version 4.3.0
//
// Do not make changes to this file unless you know what you are doing - modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class CameraCalibrationResult : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal CameraCalibrationResult(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(CameraCalibrationResult obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  internal static global::System.Runtime.InteropServices.HandleRef swigRelease(CameraCalibrationResult obj) {
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

  ~CameraCalibrationResult() {
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
          FRCVCorePINVOKE.delete_CameraCalibrationResult(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
    }
  }

  public double fx {
    set {
      FRCVCorePINVOKE.CameraCalibrationResult_fx_set(swigCPtr, value);
    } 
    get {
      double ret = FRCVCorePINVOKE.CameraCalibrationResult_fx_get(swigCPtr);
      return ret;
    } 
  }

  public double fy {
    set {
      FRCVCorePINVOKE.CameraCalibrationResult_fy_set(swigCPtr, value);
    } 
    get {
      double ret = FRCVCorePINVOKE.CameraCalibrationResult_fy_get(swigCPtr);
      return ret;
    } 
  }

  public double cx {
    set {
      FRCVCorePINVOKE.CameraCalibrationResult_cx_set(swigCPtr, value);
    } 
    get {
      double ret = FRCVCorePINVOKE.CameraCalibrationResult_cx_get(swigCPtr);
      return ret;
    } 
  }

  public double cy {
    set {
      FRCVCorePINVOKE.CameraCalibrationResult_cy_set(swigCPtr, value);
    } 
    get {
      double ret = FRCVCorePINVOKE.CameraCalibrationResult_cy_get(swigCPtr);
      return ret;
    } 
  }

  public CameraCalibrationResult() : this(FRCVCorePINVOKE.new_CameraCalibrationResult__SWIG_0(), true) {
  }

  public CameraCalibrationResult(double fx, double fy, double cx, double cy) : this(FRCVCorePINVOKE.new_CameraCalibrationResult__SWIG_1(fx, fy, cx, cy), true) {
  }

}
