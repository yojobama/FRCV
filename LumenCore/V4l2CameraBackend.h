#pragma once
#include "ICameraBackend.h"
#include <optional>
#include <vector>
#include <cstdint>

// A real V4L2 capture backend: mmap'd kernel buffers, poll()-gated DQBUF, explicit mode/exposure
// control - what CameraSource.cpp's old plain cv::VideoCapture(path, CAP_V4L2) never gave this
// project (no resolution control, no exposure control, no mode enumeration). Linux only; Windows
// uses MediaFoundationCameraBackend (or the OpenCvCameraBackend fallback) instead.
//
// MJPEG/YUYV are decoded to BGR24 in Grab() itself, not threaded through as a tagged Frame -
// Frame.cpp has no decode path for either format yet (see its own AsGray()/AsBgr() comments), so
// handing out an undecoded tagged Frame here would hand every consumer (ApriltagDetector,
// ObjectDetectionSink, ...) a format their first .AsGray()/.AsBgr() call throws on.
class V4l2CameraBackend : public ICameraBackend
{
public:
	~V4l2CameraBackend() override;

	bool Open(const std::string& devicePath) override;
	void Close() override;
	bool IsOpened() const override;
	CameraGrabResult Grab() override;
	std::string Name() const override { return "V4L2"; }

	std::vector<CameraMode> EnumerateModes() override;
	bool SetMode(const CameraMode& mode) override;
	CameraMode GetCurrentMode() const override;
	bool SetExposure(int exposureAbsolute) override;
	bool SetAutoExposure(bool enabled) override;
	bool SetGain(int gain) override;

private:
	struct MappedBuffer
	{
		void* start = nullptr;
		size_t length = 0;
	};

	// Requests kernel buffers, mmaps them, queues them all, and calls STREAMON. Idempotent -
	// no-ops if already streaming.
	bool StartStreaming();
	// Calls STREAMOFF and munmaps every buffer. Idempotent - no-ops if not streaming. Does NOT
	// close the device fd (SetMode() calls this to reconfigure format mid-session without a full
	// re-Open()).
	void StopStreaming();
	bool ApplyFormat(int width, int height, uint32_t fourcc);

	int m_Fd = -1;
	std::string m_DevicePath;
	bool m_Streaming = false;
	std::vector<MappedBuffer> m_Buffers;
	// the last mode SetMode() was actually asked to apply - GetCurrentMode()'s isNative flag
	// compares the device's real, read-back settings against this, not against whatever
	// EnumerateModes() happened to list.
	std::optional<CameraMode> m_RequestedMode;
};
