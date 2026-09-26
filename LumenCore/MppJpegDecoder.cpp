#ifdef LUMEN_WITH_MPP_JPEG
#include "MppJpegDecoder.h"

#include <rockchip/rk_mpi.h>
#include <rockchip/mpp_frame.h>
#include <rockchip/mpp_packet.h>
#include <rockchip/mpp_buffer.h>

namespace {
	// A fresh MJPEG decode context can require an "info change" round trip on its first real
	// picture (the decoder reports the buffer requirements it discovered from the bitstream
	// before it will actually produce pixels - see rockchip-linux/mpp's own test/mpi_dec_test.c,
	// the "simple decode" path this class otherwise mirrors) - MPP_DEC_SET_INFO_CHANGE_READY
	// acknowledges it and lets decoding continue, using MPP's own internally-managed frame
	// buffers (no MPP_DEC_SET_EXT_BUF_GROUP - JPEG decode has no reference-frame chaining to
	// justify an app-managed buffer pool the way H.264/HEVC decode would). Bounded, not a real
	// retry loop: one real info-change round trip is the documented case; anything beyond that
	// is treated as a protocol surprise this class doesn't understand, not looped on forever.
	constexpr int kMaxDecodeAttempts = 4;
}

MppJpegDecoder::~MppJpegDecoder()
{
	if (m_Ctx) mpp_destroy(static_cast<MppCtx>(m_Ctx));
}

bool MppJpegDecoder::EnsureInitialized()
{
	if (m_InitAttempted) return m_InitOk;
	m_InitAttempted = true;

	MppCtx ctx = nullptr;
	MppApi* api = nullptr;
	if (mpp_create(&ctx, &api) != MPP_OK) return false;
	if (mpp_init(ctx, MPP_CTX_DEC, MPP_VIDEO_CodingMJPEG) != MPP_OK) {
		mpp_destroy(ctx);
		return false;
	}

	m_Ctx = ctx;
	m_Api = api;
	m_InitOk = true;
	return true;
}

bool MppJpegDecoder::Decode(const uint8_t* jpegData, size_t jpegSize, int width, int height, bool asGray, cv::Mat& dst)
{
	if (width <= 0 || height <= 0 || !jpegData || jpegSize == 0) return false;
	if (!EnsureInitialized()) return false;

	MppApi* api = static_cast<MppApi*>(m_Api);
	MppCtx ctx = static_cast<MppCtx>(m_Ctx);

	MppPacket packet = nullptr;
	// no copy - wraps the caller's own buffer (the same V4L2 mmap'd bytes Grab() already has),
	// exactly like the software cv::imdecode path one level up.
	if (mpp_packet_init(&packet, const_cast<uint8_t*>(jpegData), jpegSize) != MPP_OK) return false;

	bool ok = false;
	for (int attempt = 0; attempt < kMaxDecodeAttempts && !ok; attempt++) {
		MppFrame frame = nullptr;
		if (api->decode(ctx, packet, &frame) != MPP_OK || !frame) break;

		if (mpp_frame_get_info_change(frame)) {
			// acknowledge and let MPP allocate its own frame buffers internally - see this file's
			// own comment above - then feed the same (already-consumed) packet again to actually
			// get pixels.
			api->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, nullptr);
			mpp_frame_deinit(&frame);
			continue;
		}

		// Only 4:2:0 (NV12) output is handled - see this class's own header comment. A decode
		// error/discarded frame, or any other reported chroma layout (4:2:2/NV16 for a 4:2:2
		// JPEG), falls back to the caller's software path rather than guessing at a conversion
		// OpenCV has no built-in cvtColor code for.
		if (mpp_frame_get_errinfo(frame) != 0 || mpp_frame_get_discard(frame) != 0 ||
			mpp_frame_get_fmt(frame) != MPP_FMT_YUV420SP) {
			mpp_frame_deinit(&frame);
			break;
		}

		int frameWidth = static_cast<int>(mpp_frame_get_width(frame));
		int frameHeight = static_cast<int>(mpp_frame_get_height(frame));
		int horStride = static_cast<int>(mpp_frame_get_hor_stride(frame));
		MppBuffer buffer = mpp_frame_get_buffer(frame);
		const uint8_t* base = buffer ? static_cast<const uint8_t*>(mpp_buffer_get_ptr(buffer)) : nullptr;

		// a genuine size mismatch (a driver/decoder surprise) falls back rather than reading a
		// mis-sized view into the real buffer - same discipline the software MJPEG path already
		// uses (see V4l2CameraBackend.cpp's own comment on this).
		if (!base || frameWidth < width || frameHeight < height || horStride < width) {
			mpp_frame_deinit(&frame);
			break;
		}

		// `dst` is already the caller's own FramePool-acquired buffer, right-sized/typed for
		// asGray - this class never touches FramePool itself, see this file's own header
		// comment on why.
		if (asGray) {
			// the Y plane IS the grayscale image - genuinely free, no colour conversion at all.
			cv::Mat yView(height, width, CV_8UC1, const_cast<uint8_t*>(base), static_cast<size_t>(horStride));
			yView.copyTo(dst);
		} else {
			// same strided-view construction V4l2CameraBackend.cpp's own software NV12 branch
			// already uses for the wire format - one Mat spanning the Y plane (height rows)
			// directly followed by interleaved UV at half resolution, all at horStride.
			cv::Mat nv12View(height * 3 / 2, width, CV_8UC1, const_cast<uint8_t*>(base), static_cast<size_t>(horStride));
			cv::cvtColor(nv12View, dst, cv::COLOR_YUV2BGR_NV12);
		}

		mpp_frame_deinit(&frame);
		ok = true;
	}

	mpp_packet_deinit(&packet);
	return ok;
}
#endif // LUMEN_WITH_MPP_JPEG
