#pragma once
#include "ICameraBackend.h"

// The always-available fallback: cv::VideoCapture, behaving identically to what
// CameraFrameSource did before this interface existed. This is currently the ONLY backend that
// exists - CameraBackendFactory::Create(AUTO, ...) resolves to it on every platform for now;
// V4l2CameraBackend/MediaFoundationCameraBackend (ROADMAP.md Phase 3) will let it become the
// construction-failure fallback instead once they land.
class OpenCvCameraBackend : public ICameraBackend
{
public:
	bool Open(const std::string& devicePath) override;
	void Close() override;
	bool IsOpened() const override;
	// preferGray is ignored - cv::VideoCapture gives no way to request a direct-to-gray decode,
	// and this backend is the non-perf-critical fallback (Windows dev machines, or a V4L2 open
	// failure), not the target of the optimization preferGray exists for.
	CameraGrabResult Grab(bool preferGray = false) override;
	std::string Name() const override { return "OpenCV"; }

	// cv::VideoCapture itself has no generic "list what this device supports" API on any backend -
	// on Windows this goes straight to Media Foundation instead (WindowsCameraEnumerator.cpp,
	// the same way device enumeration already does), bypassing OpenCV's own limitation entirely.
	// Linux's V4l2CameraBackend has always had its own real answer to this via V4L2 ioctls; this
	// was the one remaining "empty on Windows" gap between the two.
	std::vector<CameraMode> EnumerateModes() override;
	bool SetMode(const CameraMode& mode) override;
	CameraMode GetCurrentMode() const override;
	bool SetExposure(int exposureAbsolute) override;
	bool SetAutoExposure(bool enabled) override;
	bool SetGain(int gain) override;

private:
	cv::VideoCapture m_Capture;
#ifdef _WIN32
	// set by Open() when devicePath is a plain numeric index - the same value OpenWithTimeout
	// uses to open the device, and what EnumerateModes() needs to ask Media Foundation about
	// this SAME device's supported modes (cv::VideoCapture itself exposes no way to recover
	// which index it was opened with). -1 means "not opened via a numeric index" (a real device
	// symbolic link, or not opened at all yet) - EnumerateModes() has nothing to ask about then.
	int m_WindowsDeviceIndex = -1;
#endif
};
