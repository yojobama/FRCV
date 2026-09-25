#pragma once

// Pixel formats a Frame can carry, or a CameraMode can advertise. Kept intentionally small - only
// what a real producer actually emits, not every format a camera theoretically could. MJPEG/YUYV
// and the raw mono formats (Y10 onwards) exist here for CameraMode reporting only: V4l2CameraBackend
// decodes/unpacks them at capture time (to BGR24 and GRAY8 respectively), so no Frame is ever
// tagged with one.
//
// Ordinals are part of the wire contract (the REST API and webui's PIXEL_FORMAT_NAMES carry them
// as plain numbers) - append new values, never reorder.
enum class FrameFormat {
	BGR24,
	RGB24,
	GRAY8,
	NV12,
	YUYV,
	MJPEG,
	// raw mono sensor formats (global-shutter FRC cameras like the Arducam OV9281) - see
	// PixelUnpack.h for each one's exact layout
	Y10,
	Y16,
	Y10P,
	Y10BPACK,
};
