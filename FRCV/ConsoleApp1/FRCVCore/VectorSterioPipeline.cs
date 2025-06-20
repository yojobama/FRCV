//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (https://www.swig.org).
// Version 4.3.0
//
// Do not make changes to this file unless you know what you are doing - modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class VectorSterioPipeline : global::System.IDisposable, global::System.Collections.IEnumerable, global::System.Collections.Generic.IEnumerable<SterioPipeline>
 {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal VectorSterioPipeline(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(VectorSterioPipeline obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  internal static global::System.Runtime.InteropServices.HandleRef swigRelease(VectorSterioPipeline obj) {
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

  ~VectorSterioPipeline() {
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
          FRCVCorePINVOKE.delete_VectorSterioPipeline(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
    }
  }

  public VectorSterioPipeline(global::System.Collections.IEnumerable c) : this() {
    if (c == null)
      throw new global::System.ArgumentNullException("c");
    foreach (SterioPipeline element in c) {
      this.Add(element);
    }
  }

  public VectorSterioPipeline(global::System.Collections.Generic.IEnumerable<SterioPipeline> c) : this() {
    if (c == null)
      throw new global::System.ArgumentNullException("c");
    foreach (SterioPipeline element in c) {
      this.Add(element);
    }
  }

  public bool IsFixedSize {
    get {
      return false;
    }
  }

  public bool IsReadOnly {
    get {
      return false;
    }
  }

  public SterioPipeline this[int index]  {
    get {
      return getitem(index);
    }
    set {
      setitem(index, value);
    }
  }

  public int Capacity {
    get {
      return (int)capacity();
    }
    set {
      if (value < 0 || (uint)value < size())
        throw new global::System.ArgumentOutOfRangeException("Capacity");
      reserve((uint)value);
    }
  }

  public bool IsEmpty {
    get {
      return empty();
    }
  }

  public int Count {
    get {
      return (int)size();
    }
  }

  public bool IsSynchronized {
    get {
      return false;
    }
  }

  public void CopyTo(SterioPipeline[] array)
  {
    CopyTo(0, array, 0, this.Count);
  }

  public void CopyTo(SterioPipeline[] array, int arrayIndex)
  {
    CopyTo(0, array, arrayIndex, this.Count);
  }

  public void CopyTo(int index, SterioPipeline[] array, int arrayIndex, int count)
  {
    if (array == null)
      throw new global::System.ArgumentNullException("array");
    if (index < 0)
      throw new global::System.ArgumentOutOfRangeException("index", "Value is less than zero");
    if (arrayIndex < 0)
      throw new global::System.ArgumentOutOfRangeException("arrayIndex", "Value is less than zero");
    if (count < 0)
      throw new global::System.ArgumentOutOfRangeException("count", "Value is less than zero");
    if (array.Rank > 1)
      throw new global::System.ArgumentException("Multi dimensional array.", "array");
    if (index+count > this.Count || arrayIndex+count > array.Length)
      throw new global::System.ArgumentException("Number of elements to copy is too large.");
    for (int i=0; i<count; i++)
      array.SetValue(getitemcopy(index+i), arrayIndex+i);
  }

  public SterioPipeline[] ToArray() {
    SterioPipeline[] array = new SterioPipeline[this.Count];
    this.CopyTo(array);
    return array;
  }

  global::System.Collections.Generic.IEnumerator<SterioPipeline> global::System.Collections.Generic.IEnumerable<SterioPipeline>.GetEnumerator() {
    return new VectorSterioPipelineEnumerator(this);
  }

  global::System.Collections.IEnumerator global::System.Collections.IEnumerable.GetEnumerator() {
    return new VectorSterioPipelineEnumerator(this);
  }

  public VectorSterioPipelineEnumerator GetEnumerator() {
    return new VectorSterioPipelineEnumerator(this);
  }

  // Type-safe enumerator
  /// Note that the IEnumerator documentation requires an InvalidOperationException to be thrown
  /// whenever the collection is modified. This has been done for changes in the size of the
  /// collection but not when one of the elements of the collection is modified as it is a bit
  /// tricky to detect unmanaged code that modifies the collection under our feet.
  public sealed class VectorSterioPipelineEnumerator : global::System.Collections.IEnumerator
    , global::System.Collections.Generic.IEnumerator<SterioPipeline>
  {
    private VectorSterioPipeline collectionRef;
    private int currentIndex;
    private object currentObject;
    private int currentSize;

    public VectorSterioPipelineEnumerator(VectorSterioPipeline collection) {
      collectionRef = collection;
      currentIndex = -1;
      currentObject = null;
      currentSize = collectionRef.Count;
    }

    // Type-safe iterator Current
    public SterioPipeline Current {
      get {
        if (currentIndex == -1)
          throw new global::System.InvalidOperationException("Enumeration not started.");
        if (currentIndex > currentSize - 1)
          throw new global::System.InvalidOperationException("Enumeration finished.");
        if (currentObject == null)
          throw new global::System.InvalidOperationException("Collection modified.");
        return (SterioPipeline)currentObject;
      }
    }

    // Type-unsafe IEnumerator.Current
    object global::System.Collections.IEnumerator.Current {
      get {
        return Current;
      }
    }

    public bool MoveNext() {
      int size = collectionRef.Count;
      bool moveOkay = (currentIndex+1 < size) && (size == currentSize);
      if (moveOkay) {
        currentIndex++;
        currentObject = collectionRef[currentIndex];
      } else {
        currentObject = null;
      }
      return moveOkay;
    }

    public void Reset() {
      currentIndex = -1;
      currentObject = null;
      if (collectionRef.Count != currentSize) {
        throw new global::System.InvalidOperationException("Collection modified.");
      }
    }

    public void Dispose() {
        currentIndex = -1;
        currentObject = null;
    }
  }

  public VectorSterioPipeline() : this(FRCVCorePINVOKE.new_VectorSterioPipeline__SWIG_0(), true) {
  }

  public VectorSterioPipeline(VectorSterioPipeline other) : this(FRCVCorePINVOKE.new_VectorSterioPipeline__SWIG_1(VectorSterioPipeline.getCPtr(other)), true) {
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public void Clear() {
    FRCVCorePINVOKE.VectorSterioPipeline_Clear(swigCPtr);
  }

  public void Add(SterioPipeline x) {
    FRCVCorePINVOKE.VectorSterioPipeline_Add(swigCPtr, SterioPipeline.getCPtr(x));
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  private uint size() {
    uint ret = FRCVCorePINVOKE.VectorSterioPipeline_size(swigCPtr);
    return ret;
  }

  private bool empty() {
    bool ret = FRCVCorePINVOKE.VectorSterioPipeline_empty(swigCPtr);
    return ret;
  }

  private uint capacity() {
    uint ret = FRCVCorePINVOKE.VectorSterioPipeline_capacity(swigCPtr);
    return ret;
  }

  private void reserve(uint n) {
    FRCVCorePINVOKE.VectorSterioPipeline_reserve(swigCPtr, n);
  }

  public VectorSterioPipeline(int capacity) : this(FRCVCorePINVOKE.new_VectorSterioPipeline__SWIG_2(capacity), true) {
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  private SterioPipeline getitemcopy(int index) {
    SterioPipeline ret = new SterioPipeline(FRCVCorePINVOKE.VectorSterioPipeline_getitemcopy(swigCPtr, index), true);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  private SterioPipeline getitem(int index) {
    SterioPipeline ret = new SterioPipeline(FRCVCorePINVOKE.VectorSterioPipeline_getitem(swigCPtr, index), false);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  private void setitem(int index, SterioPipeline val) {
    FRCVCorePINVOKE.VectorSterioPipeline_setitem(swigCPtr, index, SterioPipeline.getCPtr(val));
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public void AddRange(VectorSterioPipeline values) {
    FRCVCorePINVOKE.VectorSterioPipeline_AddRange(swigCPtr, VectorSterioPipeline.getCPtr(values));
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public VectorSterioPipeline GetRange(int index, int count) {
    global::System.IntPtr cPtr = FRCVCorePINVOKE.VectorSterioPipeline_GetRange(swigCPtr, index, count);
    VectorSterioPipeline ret = (cPtr == global::System.IntPtr.Zero) ? null : new VectorSterioPipeline(cPtr, true);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public void Insert(int index, SterioPipeline x) {
    FRCVCorePINVOKE.VectorSterioPipeline_Insert(swigCPtr, index, SterioPipeline.getCPtr(x));
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public void InsertRange(int index, VectorSterioPipeline values) {
    FRCVCorePINVOKE.VectorSterioPipeline_InsertRange(swigCPtr, index, VectorSterioPipeline.getCPtr(values));
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public void RemoveAt(int index) {
    FRCVCorePINVOKE.VectorSterioPipeline_RemoveAt(swigCPtr, index);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public void RemoveRange(int index, int count) {
    FRCVCorePINVOKE.VectorSterioPipeline_RemoveRange(swigCPtr, index, count);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public static VectorSterioPipeline Repeat(SterioPipeline value, int count) {
    global::System.IntPtr cPtr = FRCVCorePINVOKE.VectorSterioPipeline_Repeat(SterioPipeline.getCPtr(value), count);
    VectorSterioPipeline ret = (cPtr == global::System.IntPtr.Zero) ? null : new VectorSterioPipeline(cPtr, true);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public void Reverse() {
    FRCVCorePINVOKE.VectorSterioPipeline_Reverse__SWIG_0(swigCPtr);
  }

  public void Reverse(int index, int count) {
    FRCVCorePINVOKE.VectorSterioPipeline_Reverse__SWIG_1(swigCPtr, index, count);
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

  public void SetRange(int index, VectorSterioPipeline values) {
    FRCVCorePINVOKE.VectorSterioPipeline_SetRange(swigCPtr, index, VectorSterioPipeline.getCPtr(values));
    if (FRCVCorePINVOKE.SWIGPendingException.Pending) throw FRCVCorePINVOKE.SWIGPendingException.Retrieve();
  }

}
