//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (https://www.swig.org).
// Version 4.3.0
//
// Do not make changes to this file unless you know what you are doing - modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class Logger : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal Logger(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(Logger obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  internal static global::System.Runtime.InteropServices.HandleRef swigRelease(Logger obj) {
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

  ~Logger() {
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
          FRCVCorePINVOKE.delete_Logger(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
    }
  }

  public Logger() : this(FRCVCorePINVOKE.new_Logger(), true) {
  }

  public void enterLog(string message) {
    FRCVCorePINVOKE.Logger_enterLog__SWIG_0(swigCPtr, message);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public void enterLog(LogLevel logLevel, string message) {
    FRCVCorePINVOKE.Logger_enterLog__SWIG_1(swigCPtr, (int)logLevel, message);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public void enterLog(Log log) {
    FRCVCorePINVOKE.Logger_enterLog__SWIG_2(swigCPtr, Log.getCPtr(log));
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public VectorLog GetAllLogs() {
    VectorLog ret = new VectorLog(FRCVCorePINVOKE.Logger_GetAllLogs(swigCPtr), true);
    return ret;
  }

  public VectorLog GetCertainLogs(LogLevel logLevel) {
    VectorLog ret = new VectorLog(FRCVCorePINVOKE.Logger_GetCertainLogs(swigCPtr, (int)logLevel), true);
    return ret;
  }

  public void clearAllLogs() {
    FRCVCorePINVOKE.Logger_clearAllLogs(swigCPtr);
  }

}
