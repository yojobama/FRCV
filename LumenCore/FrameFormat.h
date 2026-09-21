#pragma once

// Pixel formats a Frame can carry. Kept intentionally small - only what a real producer
// (OpenCvCameraBackend today; V4l2CameraBackend soon) actually emits, not every format a camera
// theoretically could. MJPEG/YUYV exist here for CameraMode reporting even though no backend
// decodes into a Frame carrying them yet (a hardware/software decode step lands with whichever
// backend first needs one).
enum class FrameFormat {
	BGR24,
	RGB24,
	GRAY8,
	NV12,
	YUYV,
	MJPEG,
};
