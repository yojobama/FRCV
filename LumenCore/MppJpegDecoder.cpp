#ifdef LUMEN_WITH_MPP_JPEG
#include "MppJpegDecoder.h"

#include <rockchip/rk_mpi.h>
#include <rockchip/mpp_frame.h>
#include <rockchip/mpp_packet.h>
#include <rockchip/mpp_buffer.h>

namespace {
	// A fresh MJPEG decode context requires an "info change" round trip on its first real
	// picture (the decoder reports the buffer requirements it discovered from the bitstream
	// before it will actually produce pixels - see rockchip-linux/mpp's own test/mpi_dec_test.c,
	// the "simple decode" path this class otherwise mirrors) - MPP_DEC_SET_INFO_CHANGE_READY
	// acknowledges it and lets decoding continue. CONFIRMED THE HARD WAY (a real board segfault,
	// not a guess): an explicit buffer group registered via MPP_DEC_SET_EXT_BUF_GROUP is
	// mandatory here, even for a single-frame codec with no reference chaining - a prior version
	// of this class skipped it entirely (reasoning "JPEG needs no reference-frame pool, MPP's
	// internal allocation should be enough"), which crashed the whole process the moment a real
	// frame was decoded. mpi_dec_test.c's own default buffer mode (MPP_DEC_BUF_HALF_INT in its
	// own utils/mpi_dec_utils.c) always sets one up; SetupBufferGroup below mirrors exactly that
	// path (mpp_buffer_group_get_internal + mpp_buffer_group_limit_config), not the untested
	// "mode=INTERNAL, register nothing" alternative that same file also defines but never uses
	// by default. Bounded, not a real retry loop: one real info-change round trip is the
	// documented case; anything beyond that is treated as a protocol surprise this class doesn't
	// understand, not looped on forever.
	constexpr int kMaxDecodeAttempts = 4;

	// JPEG has no reference-frame chaining (one frame in, one frame out) - a handful of buffers
	// is plenty; this only bounds how many the group is ALLOWED to grow to
	// (mpp_buffer_group_limit_config), not a fixed pre-allocation.
	constexpr RK_S32 kBufferCount = 4;
}

MppJpegDecoder::~MppJpegDecoder()
{
	// context first, then the buffer group it was using - putting the group first would free
	// memory the decoder might still touch during its own teardown.
	if (m_Ctx) mpp_destroy(static_cast<MppCtx>(m_Ctx));
	if (m_BufGroup) mpp_buffer_group_put(static_cast<MppBufferGroup>(m_BufGroup));
}

bool MppJpegDecoder::SetupBufferGroup(size_t bufSize)
{
	if (m_BufGroup && m_BufGroupSize >= bufSize) return true; // already big enough

	if (m_BufGroup) {
		mpp_buffer_group_put(static_cast<MppBufferGroup>(m_BufGroup));
		m_BufGroup = nullptr;
		m_BufGroupSize = 0;
	}

	MppApi* api = static_cast<MppApi*>(m_Api);
	MppCtx ctx = static_cast<MppCtx>(m_Ctx);

	// priority order per mpp_buffer.h's own comment ("MPP_BUFFER_TYPE_DMA_HEAP >
	// MPP_BUFFER_TYPE_DRM > MPP_BUFFER_TYPE_ION") - fall back down the list if the preferred
	// allocator isn't available on this kernel rather than failing outright.
	static const MppBufferType kTypesInPriorityOrder[] = {
		MPP_BUFFER_TYPE_DMA_HEAP, MPP_BUFFER_TYPE_DRM, MPP_BUFFER_TYPE_ION
	};
	MppBufferGroup group = nullptr;
	for (MppBufferType type : kTypesInPriorityOrder) {
		if (mpp_buffer_group_get(&group, type, MPP_BUFFER_INTERNAL, "lumen_mpp_jpeg", __func__) == MPP_OK && group) break;
		group = nullptr;
	}
	if (!group) return false;

	if (mpp_buffer_group_limit_config(group, bufSize, kBufferCount) != MPP_OK) {
		mpp_buffer_group_put(group);
		return false;
	}
	if (api->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, group) != MPP_OK) {
		mpp_buffer_group_put(group);
		return false;
	}

	m_BufGroup = group;
	m_BufGroupSize = bufSize;
	return true;
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
			// set up (or grow) the buffer group the decoder needs BEFORE acknowledging - see this
			// file's own top comment on why this is mandatory, not optional. A failure here (no
			// supported allocator, group setup rejected) falls straight back to software rather
			// than acknowledging into a decoder that has nowhere to put its output.
			if (!SetupBufferGroup(mpp_frame_get_buf_size(frame))) {
				mpp_frame_deinit(&frame);
				break;
			}
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
