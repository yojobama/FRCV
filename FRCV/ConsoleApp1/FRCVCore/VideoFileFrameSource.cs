//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (https://www.swig.org).
// Version 4.3.0
//
// Do not make changes to this file unless you know what you are doing - modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class VideoFileFrameSource : IFrameSource {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;

  internal VideoFileFrameSource(global::System.IntPtr cPtr, bool cMemoryOwn) : base(FRCVCorePINVOKE.VideoFileFrameSource_SWIGUpcast(cPtr), cMemoryOwn) {
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(VideoFileFrameSource obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  internal static global::System.Runtime.InteropServices.HandleRef swigRelease(VideoFileFrameSource obj) {
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

  protected override void Dispose(bool disposing) {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          FRCVCorePINVOKE.delete_VideoFileFrameSource(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      base.Dispose(disposing);
    }
  }

  public VideoFileFrameSource(Logger logger, string filePath) : this(FRCVCorePINVOKE.new_VideoFileFrameSource(Logger.getCPtr(logger), filePath), true) {
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public override Frame getFrame() {
    global::System.IntPtr cPtr = FRCVCorePINVOKE.VideoFileFrameSource_getFrame(swigCPtr);
    Frame ret = (cPtr == global::System.IntPtr.Zero) ? null : new Frame(cPtr, false);
    return ret;
  }

}
