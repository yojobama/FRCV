#pragma once
#include <opencv2/core.hpp>
#include <cstddef>
#include <cstdint>

// Raw-sensor mono formats -> 8-bit grayscale. What global-shutter FRC cameras (Arducam OV9281 and
// friends) hand out when they're not behind a UVC bridge that already converts to YUYV/MJPEG:
// V4L2's Y10/Y16/Y10P/Y10BPACK. Nothing downstream (AprilTag, calibration, encoders) wants more
// than 8 bits, so every path here keeps the most significant 8 bits rather than threading a
// 16-bit Frame through the pipeline.
//
// Deliberately free of <linux/videodev2.h> so the conversions build (and are unit-tested) on
// every platform, not only where V4l2CameraBackend itself compiles. `stride` is the driver's
// bytesperline - it can exceed width * bytes-per-pixel (row padding), so rows are never assumed
// to be contiguous. `dst` is (re)created as CV_8UC1 width x height; an already-right-sized dst
// (e.g. a FramePool buffer) is written in place.
namespace PixelUnpack
{
	// V4L2_PIX_FMT_Y10: one little-endian 16-bit word per pixel, value in the low 10 bits.
	void Y10ToGray8(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst);

	// V4L2_PIX_FMT_Y16: one little-endian 16-bit word per pixel, full 16-bit range.
	void Y16ToGray8(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst);

	// V4L2_PIX_FMT_Y10P (MIPI CSI-2 RAW10 packing - the Raspberry Pi/unicam OV9281 format): every
	// 4 pixels are 5 bytes, the first 4 holding each pixel's 8 MSBs and the 5th their 2 LSBs.
	void Y10PToGray8(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst);

	// V4L2_PIX_FMT_Y10BPACK: a big-endian 10-bit bit stream - 4 pixels in 5 bytes, but each
	// pixel's bits run straight across byte boundaries (unlike Y10P's MSB bytes + LSB byte).
	void Y10BPackToGray8(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst);

	// V4L2_PIX_FMT_GREY with arbitrary stride - a plain copy, row by row.
	void Gray8Copy(const uint8_t* data, int width, int height, size_t stride, cv::Mat& dst);

	// The minimum bytesperline for `width` pixels in each format - what the V4L2 spec says a
	// driver must report at least, used to reject a truncated buffer before reading past it.
	size_t MinStrideY10(int width);  // also Y16
	size_t MinStrideY10Packed(int width); // Y10P and Y10BPACK
}
