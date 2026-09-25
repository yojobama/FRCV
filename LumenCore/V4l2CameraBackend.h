#pragma once
#include "ICameraBackend.h"
#include <optional>
#include <vector>
#include <cstdint>
#include <initializer_list>

// A real V4L2 capture backend: mmap'd kernel buffers, poll()-gated DQBUF, explicit mode/exposure
// control - what CameraSource.cpp's old plain cv::VideoCapture(path, CAP_V4L2) never gave this
// project (no resolution control, no exposure control, no mode enumeration). Linux only; Windows
// uses MediaFoundationCameraBackend (or the OpenCvCameraBackend fallback) instead.
//
// MJPEG/YUYV/NV12 are decoded to BGR24 in Grab() itself, not threaded through as a tagged Frame -
// Frame.cpp has no decode path for them (see its own AsGray()/AsBgr() comments), so handing out
// an undecoded tagged Frame here would hand every consumer (ApriltagDetector,
// ObjectDetectionSink, ...) a format their first .AsGray()/.AsBgr() call throws on. Mono formats
// (GREY, and the raw Y10/Y16/Y10P/Y10BPACK a global-shutter Arducam OV9281 exposes) come out as
// GRAY8 instead - see PixelUnpack.h.
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
	CameraControlRange GetExposureRange() override;
	CameraControlRange GetGainRange() override;

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
	// The first of `candidates` this device actually implements (VIDIOC_QUERYCTRL succeeds and it
	// isn't flagged disabled), or 0 if none - UVC webcams expose EXPOSURE_ABSOLUTE/GAIN, raw
	// sensor drivers (Arducam MIPI modules, some Arducam UVC firmwares) EXPOSURE/ANALOGUE_GAIN.
	uint32_t FindControl(std::initializer_list<uint32_t> candidates) const;
	CameraControlRange QueryControlRange(uint32_t cid) const;

	int m_Fd = -1;
	std::string m_DevicePath;
	bool m_Streaming = false;
	std::vector<MappedBuffer> m_Buffers;
	// the last mode SetMode() was actually asked to apply - GetCurrentMode()'s isNative flag
	// compares the device's real, read-back settings against this, not against whatever
	// EnumerateModes() happened to list.
	std::optional<CameraMode> m_RequestedMode;
};
