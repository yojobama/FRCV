#include "Frame.h"
#include "FramePool.h"
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

Frame::Frame(cv::Mat mat, FrameFormat format, std::shared_ptr<void> poolOwner)
	: m_Storage(std::make_shared<Storage>())
{
	m_Storage->mat = std::move(mat);
	m_Storage->format = format;
	m_Storage->matPoolOwner = std::move(poolOwner);
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
		std::shared_ptr<void> owner;
		switch (m_Storage->format) {
		case FrameFormat::BGR24:
			// Acquire()d at the exact target shape BEFORE the conversion, not after: cv::cvtColor
			// only skips its own internal allocation (Mat::create's fast path) when `gray`
			// already has the right rows/cols/type going in - an uninitialized `cv::Mat gray;`
			// forces a fresh allocation every single call, every frame, which is the whole
			// per-frame cost FramePool exists to remove from this exact hot path.
			gray = FramePool::Instance().Acquire(m_Storage->mat.rows, m_Storage->mat.cols, CV_8UC1, owner);
			cv::cvtColor(m_Storage->mat, gray, cv::COLOR_BGR2GRAY);
			break;
		case FrameFormat::RGB24:
			gray = FramePool::Instance().Acquire(m_Storage->mat.rows, m_Storage->mat.cols, CV_8UC1, owner);
			cv::cvtColor(m_Storage->mat, gray, cv::COLOR_RGB2GRAY);
			break;
		case FrameFormat::NV12:
			// NV12's Y plane is the top 2/3 of the packed buffer (a WxH image's NV12 data is
			// W x (H*3/2), Y first then interleaved UV at half resolution) - no backend
			// produces an NV12-tagged Frame yet (V4l2CameraBackend, ROADMAP.md Phase 3), so
			// this is unreached/unverified today; written to the standard packing convention
			// for when it lands rather than left unimplemented. Not pooled - not worth wiring
			// up a buffer-recycling path for code nothing exercises yet.
			gray = m_Storage->mat(cv::Rect(0, 0, m_Storage->mat.cols, m_Storage->mat.rows * 2 / 3)).clone();
			break;
		default:
			// MJPEG/YUYV need a real decode step this project has no backend for yet -
			// failing loudly here beats silently handing a caller garbage pixels.
			throw std::runtime_error("Frame::AsGray: no conversion implemented for this format");
		}
		m_Storage->grayView = gray;
		m_Storage->grayPoolOwner = std::move(owner);
	}
	return *m_Storage->grayView;
}

Frame Frame::AsBgrFrame() const
{
	if (!m_Storage) throw std::runtime_error("Frame::AsBgrFrame called on an empty (default-constructed) Frame");
	const cv::Mat& bgr = AsBgr(); // populates the cache first (a no-op if already BGR24 or cached)

	std::lock_guard<std::mutex> guard(m_Storage->viewMutex);
	std::shared_ptr<void> owner = (m_Storage->format == FrameFormat::BGR24) ? m_Storage->matPoolOwner : m_Storage->bgrPoolOwner;
	return Frame(bgr, FrameFormat::BGR24, owner);
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
		std::shared_ptr<void> owner;
		switch (m_Storage->format) {
		case FrameFormat::RGB24:
			bgr = FramePool::Instance().Acquire(m_Storage->mat.rows, m_Storage->mat.cols, CV_8UC3, owner);
			cv::cvtColor(m_Storage->mat, bgr, cv::COLOR_RGB2BGR);
			break;
		case FrameFormat::GRAY8:
			bgr = FramePool::Instance().Acquire(m_Storage->mat.rows, m_Storage->mat.cols, CV_8UC3, owner);
			cv::cvtColor(m_Storage->mat, bgr, cv::COLOR_GRAY2BGR);
			break;
		default:
			throw std::runtime_error("Frame::AsBgr: no conversion implemented for this format");
		}
		m_Storage->bgrView = bgr;
		m_Storage->bgrPoolOwner = std::move(owner);
	}
	return *m_Storage->bgrView;
}
