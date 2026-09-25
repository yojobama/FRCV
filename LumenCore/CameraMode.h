#pragma once
#include <string>
#include "FrameFormat.h"

// A single capture mode a camera device can be asked to produce, and (via isNative) whether a
// request for it was actually honoured. Both V4L2 and Media Foundation silently substitute a
// nearest mode when asked for one a device doesn't have - reporting success either way - so
// isNative is only meaningful on a CameraMode returned by ICameraBackend::GetCurrentMode() after
// a SetMode() call, never on one returned by EnumerateModes() (every enumerated mode is, by
// construction, one the device actually advertises).
struct CameraMode
{
	int width = 0;
	int height = 0;
	// frames per second, not an interval - a discrete V4L2 frame interval of 1/30s becomes 30.0
	// here, matching what the mode-picker UI and the REST API both want to show directly.
	double fps = 0.0;
	// the wire format this mode captures in. Almost always MJPEG or YUYV for a UVC webcam; note
	// this is NOT necessarily what a Frame built from this mode is tagged as - V4l2CameraBackend
	// decodes MJPEG/YUYV to BGR24 at capture time today (Frame.cpp has no decode path for either
	// yet), so this field on a CameraMode describes device capability, not delivered pixel data.
	FrameFormat pixelFormat = FrameFormat::MJPEG;
	// true only on a CameraMode returned by GetCurrentMode() when the device's actual settings,
	// read back after SetMode(), exactly match what was requested.
	bool isNative = false;
};

// A camera control's real range as the device itself reports it (V4L2 VIDIOC_QUERYCTRL), so the
// UI can bound its inputs to what this camera accepts instead of assuming UVC-webcam conventions -
// an Arducam's exposure is in sensor lines with a range nothing like a webcam's 100us units.
// supported == false (every other field then meaningless) when the device/backend has no such
// control at all.
struct CameraControlRange
{
	bool supported = false;
	int minimum = 0;
	int maximum = 0;
	int step = 1;
	int defaultValue = 0;
	// the control's value right now (VIDIOC_G_CTRL), so the UI starts from what's applied
	int value = 0;
};
