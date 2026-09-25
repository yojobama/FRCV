#pragma once
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "CameraMode.h"

// A single captured frame plus the instant it was captured. Deliberately still cv::Mat-based
// (not the format-tagged Frame type ROADMAP.md Phase 3 describes) so this interface can land on
// its own, with CameraFrameSource rewritten onto it, before Frame/SourceResult are touched in a
// separate change.
struct CameraGrabResult
{
	bool success = false;
	cv::Mat frame;
	uint64_t captureTimeUs = 0;
	// non-null when `frame` is backed by a FramePool buffer (V4l2CameraBackend::Grab() sets this;
	// OpenCvCameraBackend leaves it null, which is exactly as correct - a null poolOwner just
	// means `frame` owns its own memory the normal cv::Mat way). CameraFrameSource::CaptureFrame
	// must carry this through to the Frame it constructs from `frame` - see Frame's own
	// pool-owner-taking constructor - or the pooled buffer can be recycled out from under a Frame
	// still using it the moment this shared_ptr's last reference (this struct, once Grab()
	// returns) goes away.
	std::shared_ptr<void> poolOwner;
	// what `frame` actually holds - BGR24 (CV_8UC3) or GRAY8 (CV_8UC1). A mono camera's frames
	// stay single-channel all the way to the AprilTag detector (whose AsGray() is then free)
	// instead of being expanded to BGR here and converted straight back there; sinks that need
	// colour convert lazily via Frame::AsBgr().
	FrameFormat format = FrameFormat::BGR24;
};

// Implemented per-platform: OpenCvCameraBackend is the always-available fallback (the only one
// that exists right now); V4l2CameraBackend (Linux) and MediaFoundationCameraBackend (Windows)
// are ROADMAP.md Phase 3 follow-ups. CameraFrameSource owns one of these rather than a
// cv::VideoCapture directly, so swapping backends later doesn't touch ISource plumbing at all.
class ICameraBackend
{
public:
	virtual ~ICameraBackend() = default;

	// Opens the device at devicePath. Returns false rather than throwing - the caller decides
	// whether a failed open is fatal.
	virtual bool Open(const std::string& devicePath) = 0;
	virtual void Close() = 0;
	virtual bool IsOpened() const = 0;

	// Must return within a bounded time even if the device stops producing frames (e.g.
	// unplugged mid-capture) - ISource::Toggle(false) blocks joining the capture thread, so a
	// backend that blocks forever here hangs StopSourceById/DeleteSource/process shutdown
	// indefinitely. A real V4L2 backend needs poll() with a timeout ahead of VIDIOC_DQBUF; a
	// real Media Foundation backend needs Close() to call IMFSourceReader::Flush to unblock a
	// pending ReadSample from another thread.
	virtual CameraGrabResult Grab() = 0;

	virtual std::string Name() const = 0;

	// Real device capabilities, queried after Open(). Empty on a backend with no way to enumerate
	// them at all - none exist today: V4l2CameraBackend (CameraBackendFactory's preferred Linux
	// backend) answers this via V4L2 ioctls, and OpenCvCameraBackend (the fallback there, and the
	// only backend on every other platform including Windows) goes straight to Media Foundation
	// on Windows specifically, bypassing cv::VideoCapture's own lack of a generic capability
	// query - see OpenCvCameraBackend's own comment.
	virtual std::vector<CameraMode> EnumerateModes() = 0;

	// Both V4L2 and Media Foundation silently substitute a nearest mode rather than failing on a
	// request they can't satisfy exactly - callers MUST re-read GetCurrentMode() afterwards
	// (checking its isNative flag) rather than trusting this return value's true as "got exactly
	// what was asked for". This return value only reports whether the underlying ioctl/API call
	// itself succeeded.
	virtual bool SetMode(const CameraMode& mode) = 0;
	virtual CameraMode GetCurrentMode() const = 0;

	// Exposure/gain control - a fixed short exposure is what actually makes AprilTags detect
	// reliably on a moving robot (motion blur otherwise smears the tag edges the detector needs).
	// exposureAbsolute is in the backend's own native units (V4L2: 100us steps, matching
	// V4L2_CID_EXPOSURE_ABSOLUTE's documented convention - or, on a sensor that only has
	// V4L2_CID_EXPOSURE, e.g. many Arducam modules, that control's own units, typically lines; see
	// GetExposureRange for what the device accepts). Returns false if the control isn't
	// supported by this device/backend rather than throwing - an unsupported control is routine,
	// not exceptional.
	virtual bool SetExposure(int exposureAbsolute) = 0;
	virtual bool SetAutoExposure(bool enabled) = 0;
	virtual bool SetGain(int gain) = 0;

	// The device's real range for the control SetExposure/SetGain drive - see CameraControlRange.
	// Default: unsupported, for a backend with no way to query it (OpenCvCameraBackend).
	virtual CameraControlRange GetExposureRange() { return {}; }
	virtual CameraControlRange GetGainRange() { return {}; }
};
