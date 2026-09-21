#include "Frame.h"
#include <stdexcept>

Frame::Frame(const cv::Mat& mat)
	: Frame(mat, FrameFormat::BGR24)
{
}

Frame::Frame(cv::Mat mat, FrameFormat format)
	: m_Storage(std::make_shared<Storage>())
{
	m_Storage->mat = std::move(mat);
	m_Storage->format = format;
}

bool Frame::empty() const
{
	return !m_Storage || m_Storage->mat.empty();
}

cv::Size Frame::size() const
{
	return m_Storage ? m_Storage->mat.size() : cv::Size();
}

FrameFormat Frame::format() const
{
	return m_Storage ? m_Storage->format : FrameFormat::BGR24;
}

Frame Frame::Roi(const cv::Rect& roi) const
{
	if (!m_Storage) throw std::runtime_error("Frame::Roi called on an empty (default-constructed) Frame");
	// cv::Mat's own ROI constructor is itself zero-copy (a new header sharing the same refcounted
	// data), which is exactly what this is built on - no pixel data is duplicated here.
	return Frame(m_Storage->mat(roi), m_Storage->format);
}

const cv::Mat& Frame::AsGray() const
{
	if (!m_Storage) throw std::runtime_error("Frame::AsGray called on an empty (default-constructed) Frame");

	std::lock_guard<std::mutex> guard(m_Storage->viewMutex);
	if (m_Storage->format == FrameFormat::GRAY8) {
		return m_Storage->mat;
	}
	if (!m_Storage->grayView.has_value()) {
		cv::Mat gray;
		switch (m_Storage->format) {
		case FrameFormat::BGR24:
			cv::cvtColor(m_Storage->mat, gray, cv::COLOR_BGR2GRAY);
			break;
		case FrameFormat::RGB24:
			cv::cvtColor(m_Storage->mat, gray, cv::COLOR_RGB2GRAY);
			break;
		case FrameFormat::NV12:
			// NV12's Y plane is the top 2/3 of the packed buffer (a WxH image's NV12 data is
			// W x (H*3/2), Y first then interleaved UV at half resolution) - no backend
			// produces an NV12-tagged Frame yet (V4l2CameraBackend, ROADMAP.md Phase 3), so
			// this is unreached/unverified today; written to the standard packing convention
			// for when it lands rather than left unimplemented.
			gray = m_Storage->mat(cv::Rect(0, 0, m_Storage->mat.cols, m_Storage->mat.rows * 2 / 3)).clone();
			break;
		default:
			// MJPEG/YUYV need a real decode step this project has no backend for yet -
			// failing loudly here beats silently handing a caller garbage pixels.
			throw std::runtime_error("Frame::AsGray: no conversion implemented for this format");
		}
		m_Storage->grayView = gray;
	}
	return *m_Storage->grayView;
}

const cv::Mat& Frame::AsBgr() const
{
	if (!m_Storage) throw std::runtime_error("Frame::AsBgr called on an empty (default-constructed) Frame");

	std::lock_guard<std::mutex> guard(m_Storage->viewMutex);
	if (m_Storage->format == FrameFormat::BGR24) {
		return m_Storage->mat;
	}
	if (!m_Storage->bgrView.has_value()) {
		cv::Mat bgr;
		switch (m_Storage->format) {
		case FrameFormat::RGB24:
			cv::cvtColor(m_Storage->mat, bgr, cv::COLOR_RGB2BGR);
			break;
		case FrameFormat::GRAY8:
			cv::cvtColor(m_Storage->mat, bgr, cv::COLOR_GRAY2BGR);
			break;
		default:
			throw std::runtime_error("Frame::AsBgr: no conversion implemented for this format");
		}
		m_Storage->bgrView = bgr;
	}
	return *m_Storage->bgrView;
}
