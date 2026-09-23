#pragma once
#ifdef _WIN32

#include <memory>
#include <string>
#include <vector>

class Logger;

// Separated out from Manager.cpp on purpose: Manager.h brings `using namespace std;` into scope,
// and the Windows SDK's COM/RPC headers (rpcndr.h, objidl.h, wtypes.h, ...) that mfapi.h/mfidl.h
// transitively pull in reference an unqualified `byte` of their own - with std::byte ALSO in
// unqualified lookup because of that using-directive, every one of those references becomes a
// real ambiguous-symbol compile error (confirmed the hard way: dozens of C2872s the moment
// mfapi.h was included straight into Manager.cpp). This translation unit has no `using namespace
// std;` anywhere, so the ambiguity never arises.
struct WindowsCameraDevice {
    std::string name;
    int index; // MFEnumDeviceSources' array position - see EnumerateWindowsCameras' own comment
};

// Enumerates video capture devices via Media Foundation's MFEnumDeviceSources - the same
// underlying API the Windows Camera app and Device Manager use, so what this returns should
// match what a user sees there. Logger is optional (nullable) purely so this can be unit-tested
// without constructing one.
std::vector<WindowsCameraDevice> EnumerateWindowsCameras(const std::shared_ptr<Logger>& logger);

#endif // _WIN32
