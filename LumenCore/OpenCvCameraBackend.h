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
	CameraGrabResult Grab() override;
	std::string Name() const override { return "OpenCV"; }

	// cv::VideoCapture has no generic "list what this device supports" API on any backend - always
	// empty. GetCurrentMode()/SetMode() still work (via CAP_PROP_FRAME_WIDTH/HEIGHT/FPS), they are
	// just not driven by a prior enumeration here.
	std::vector<CameraMode> EnumerateModes() override;
	bool SetMode(const CameraMode& mode) override;
	CameraMode GetCurrentMode() const override;
	bool SetExposure(int exposureAbsolute) override;
	bool SetAutoExposure(bool enabled) override;
	bool SetGain(int gain) override;

private:
	cv::VideoCapture m_Capture;
};
