#pragma once

// Split out to its own header for the same reason as CalibrationBoardType.h: Manager.h exposes
// this through SWIG, and a plain (unscoped) enum is wrapped as a real C# enum, while a scoped
// `enum class` falls back to a broken opaque SWIGTYPE_p_* handle - confirmed three times already
// in this codebase (YoloVariant, ApriltagBackendKind, CalibrationBoardType).
//
// See STEREO_IMPLEMENTATION_PLAN.md ss0 for why STEREO_BACKEND_CODEC_RKMPP is deliberately not
// an option here: codec-stereo's own design doc documents confirmed silicon/firmware defects in
// that backend's KEY_MOTION_INFO readback (only the top half of a frame's rows get written, odd
// 16px columns stuck at a placeholder MV) - rkmpp_hwenc sidesteps it entirely by decoding the
// real bitstream instead, and is the one this project selects on the Orange Pi.
enum StereoDepthBackendKind {
	STEREO_BACKEND_CODEC_AUTO,          // cs_init() auto-probe (never picks rkmpp - see above)
	STEREO_BACKEND_CODEC_LAVC,          // codec-stereo backend_override = "lavc_sw"
	STEREO_BACKEND_CODEC_RKMPP_HWENC,   // codec-stereo backend_override = "rkmpp_hwenc"
	STEREO_BACKEND_SGBM                 // cv::StereoSGBM, block-aggregated to the same grid
};
