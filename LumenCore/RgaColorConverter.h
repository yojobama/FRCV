#pragma once
#ifdef LUMEN_WITH_RGA

#include <opencv2/opencv.hpp>
#include <vector>

extern "C" {
#include <libavutil/frame.h>
}

// Offloads WebRTCSink's BGR24->NV12 colour conversion to the RK3588's RGA (2D raster graphics
// accelerator) via librga's im2d C API, instead of doing it entirely on the CPU (libswscale)
// every single encoded frame - real CPU/latency cost on every WebRTC preview frame, not just a
// theoretical one, since WebRTCSink runs on the same processing thread as everything else bound
// to that sink's source.
//
// rga_buffer_t (librga's own buffer descriptor - see im2d_type.h) carries exactly ONE virtual
// address per call, with the second/third plane of a multi-plane format computed internally from
// wstride*hstride - there is no way to hand it two separately-strided plane pointers. This
// matters because ffmpeg's own av_frame_get_buffer does NOT reliably lay out NV12's Y/UV planes
// at that exact wstride*hstride offset (confirmed empirically on the actual Orange Pi: a 640x480
// frame came back with a 32-byte gap between planes that plain arithmetic didn't predict,
// regardless of the alignment value passed to av_frame_get_buffer) - writing RGA's output
// directly into an AVFrame's own buffer is silently wrong exactly when the two layouts disagree.
// ConvertBgrToNv12 therefore converts into its own tightly-packed scratch buffer (the layout RGA
// actually produces) and copies each plane into the destination AVFrame via av_image_copy_plane,
// which correctly handles the destination's real (possibly different) linesize - still far
// cheaper than the CPU YUV conversion this replaces, since the copy itself is a fast memcpy, not
// per-pixel colour-space math.
class RgaColorConverter {
public:
	// Converts bgrFrame (CV_8UC3, BGR order) into dstFrame, which must already be allocated as
	// AV_PIX_FMT_NV12 at bgrFrame's own width/height. Returns false on any RGA failure
	// (unsupported size/stride, driver error, hardware busy/absent) - the caller falls back to
	// sws_scale rather than treating this as fatal, matching how ApriltagDetector falls back
	// from Vulkan to CPU on its own hardware-path failure.
	bool ConvertBgrToNv12(const cv::Mat& bgrFrame, AVFrame* dstFrame);

private:
	// reused across calls, only reallocated when the frame size actually changes - avoids a
	// fresh heap allocation on every single frame.
	std::vector<uint8_t> m_Scratch;
};

#endif // LUMEN_WITH_RGA
