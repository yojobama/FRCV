#ifdef _WIN32

#include "WindowsCameraEnumerator.h"
#include "Logger.h"

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>

std::vector<WindowsCameraDevice> EnumerateWindowsCameras(const std::shared_ptr<Logger>& logger)
{
    std::vector<WindowsCameraDevice> cameras;

    // MFSTARTUP_LITE skips Media Foundation's platform pipeline (mfplat.dll's own transform
    // registry) - not needed just to enumerate device sources, and it starts up meaningfully
    // faster than a full MFStartup(MF_VERSION) would.
    HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
    if (FAILED(hr)) {
        if (logger) logger->EnterLog(LogLevel::Error, "EnumerateWindowsCameras: MFStartup failed, hr=" + std::to_string(hr));
        return cameras;
    }

    IMFAttributes* pAttributes = nullptr;
    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;

    hr = MFCreateAttributes(&pAttributes, 1);
    if (SUCCEEDED(hr)) {
        hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    }
    if (SUCCEEDED(hr)) {
        hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    }

    if (SUCCEEDED(hr)) {
        for (UINT32 i = 0; i < count; i++) {
            WCHAR* friendlyName = nullptr;
            UINT32 nameLength = 0;
            HRESULT nameHr = ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendlyName, &nameLength);

            std::string name;
            if (SUCCEEDED(nameHr) && friendlyName != nullptr) {
                int utf8Length = WideCharToMultiByte(CP_UTF8, 0, friendlyName, -1, nullptr, 0, nullptr, nullptr);
                if (utf8Length > 0) {
                    name.resize(static_cast<size_t>(utf8Length) - 1);
                    WideCharToMultiByte(CP_UTF8, 0, friendlyName, -1, name.data(), utf8Length, nullptr, nullptr);
                }
                CoTaskMemFree(friendlyName);
            }
            if (name.empty()) {
                name = "Camera " + std::to_string(i);
            }

            cameras.push_back(WindowsCameraDevice{ name, static_cast<int>(i) });
            if (logger) logger->EnterLog("EnumerateWindowsCameras: found \"" + name + "\" at index " + std::to_string(i));

            ppDevices[i]->Release();
        }
        CoTaskMemFree(ppDevices);
    } else {
        if (logger) logger->EnterLog(LogLevel::Error, "EnumerateWindowsCameras: MFEnumDeviceSources failed, hr=" + std::to_string(hr));
    }

    if (pAttributes != nullptr) pAttributes->Release();
    MFShutdown();

    return cameras;
}

#endif // _WIN32
