#ifdef LUMEN_WITH_RGA
#include "RgaColorConverter.h"

// librga's own headers assume NULL is already defined by an includer before them (confirmed the
// hard way - they don't include <cstddef>/<cstdlib> themselves and fail to compile standalone
// without this) - keep this include first, not folded into im2d.h's own guard.
#include <cstddef>
#include <im2d.h>

extern "C" {
#include <libavutil/imgutils.h>
}

bool RgaColorConverter::ConvertBgrToNv12(const cv::Mat& bgrFrame, AVFrame* dstFrame)
{
	int width = bgrFrame.cols;
	int height = bgrFrame.rows;

	size_t scratchSize = static_cast<size_t>(width) * height * 3 / 2;
	if (m_Scratch.size() != scratchSize) m_Scratch.resize(scratchSize);

	// wstride in PIXELS, not bytes (confirmed against wrapbuffer_virtualaddr's own default-args
	// macro, which passes plain `width` when no stride override is given) - bgrFrame.step is in
	// bytes, so it must be divided by elemSize() for a non-tightly-packed source (e.g. an ROI
	// view via Frame::Roi()) to convert correctly rather than silently reading skewed rows.
	int srcWstride = static_cast<int>(bgrFrame.step / bgrFrame.elemSize());
	rga_buffer_t src = wrapbuffer_virtualaddr_t(
		const_cast<uchar*>(bgrFrame.data), width, height, srcWstride, height, RK_FORMAT_BGR_888);
	// tightly packed (wstride=width) - this is the layout RGA itself produces internally for a
	// planar/semi-planar destination given a single vir_addr, verified empirically (see this
	// class's own header comment) - NOT necessarily what dstFrame's own linesize[0]/[1] are.
	rga_buffer_t dst = wrapbuffer_virtualaddr_t(
		m_Scratch.data(), width, height, width, height, RK_FORMAT_YCbCr_420_SP);

	// BT601 limited range - matches libswscale's own default when no colorspace is explicitly
	// requested (the fallback path this replaces), so switching between RGA/sws_scale frame to
	// frame (on a transient RGA failure) doesn't visibly shift colours.
	IM_STATUS status = imcvtcolor(src, dst, RK_FORMAT_BGR_888, RK_FORMAT_YCbCr_420_SP, IM_RGB_TO_YUV_BT601_LIMIT);
	if (status != IM_STATUS_SUCCESS) return false;

	av_image_copy_plane(dstFrame->data[0], dstFrame->linesize[0], m_Scratch.data(), width, width, height);
	av_image_copy_plane(dstFrame->data[1], dstFrame->linesize[1], m_Scratch.data() + static_cast<size_t>(width) * height, width, width, height / 2);
	return true;
}

#endif // LUMEN_WITH_RGA
