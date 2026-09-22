#include "V4l2CameraBackend.h"
#include "SourceResult.h"
#include "FramePool.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <linux/videodev2.h>
#include <cstring>
#include <errno.h>

namespace {
	constexpr int BUFFER_COUNT = 4;
	// bounded well above any real frame interval (even 5fps is 200ms) so Grab() never mistakes a
	// slow-but-alive camera for a hung one, while still returning promptly - see ICameraBackend's
	// own comment on why this bound matters for ISource::Toggle(false)/shutdown.
	constexpr int POLL_TIMEOUT_MS = 1000;

	int XIoctl(int fd, unsigned long request, void* arg)
	{
		int ret;
		// EINTR is routine (a signal interrupting the syscall, not a real failure) - retry rather
		// than surfacing it as an ioctl error to the caller.
		do {
			ret = ioctl(fd, request, arg);
		} while (ret == -1 && errno == EINTR);
		return ret;
	}

	FrameFormat FourCcToFrameFormat(uint32_t fourcc)
	{
		switch (fourcc) {
		case V4L2_PIX_FMT_MJPEG:
		case V4L2_PIX_FMT_JPEG:
			return FrameFormat::MJPEG;
		case V4L2_PIX_FMT_YUYV:
			return FrameFormat::YUYV;
		case V4L2_PIX_FMT_NV12:
			return FrameFormat::NV12;
		case V4L2_PIX_FMT_BGR24:
			return FrameFormat::BGR24;
		case V4L2_PIX_FMT_RGB24:
			return FrameFormat::RGB24;
		case V4L2_PIX_FMT_GREY:
			return FrameFormat::GRAY8;
		default:
			// no first-party consumer asks for anything else today - MJPEG is the closest honest
			// default for "some compressed/unrecognised format", not a claim this fourcc IS MJPEG.
			return FrameFormat::MJPEG;
		}
	}

	uint32_t FrameFormatToFourCc(FrameFormat format)
	{
		switch (format) {
		case FrameFormat::MJPEG: return V4L2_PIX_FMT_MJPEG;
		case FrameFormat::YUYV: return V4L2_PIX_FMT_YUYV;
		case FrameFormat::NV12: return V4L2_PIX_FMT_NV12;
		case FrameFormat::BGR24: return V4L2_PIX_FMT_BGR24;
		case FrameFormat::RGB24: return V4L2_PIX_FMT_RGB24;
		case FrameFormat::GRAY8: return V4L2_PIX_FMT_GREY;
		default: return V4L2_PIX_FMT_MJPEG;
		}
	}
}

V4l2CameraBackend::~V4l2CameraBackend()
{
	Close();
}

bool V4l2CameraBackend::Open(const std::string& devicePath)
{
	Close();

	// O_NONBLOCK matters here even though Grab() itself polls with a timeout before DQBUF - a
	// blocking fd would make VIDIOC_DQBUF itself block indefinitely if a spurious poll() wakeup
	// ever raced an empty queue, defeating the whole point of the poll-first bound.
	m_Fd = open(devicePath.c_str(), O_RDWR | O_NONBLOCK);
	if (m_Fd < 0) return false;

	v4l2_capability cap{};
	if (XIoctl(m_Fd, VIDIOC_QUERYCAP, &cap) < 0 ||
		!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
		!(cap.capabilities & V4L2_CAP_STREAMING)) {
		close(m_Fd);
		m_Fd = -1;
		return false;
	}

	m_DevicePath = devicePath;

	// A sane default so a caller that never calls SetMode() still gets frames - MJPEG at a modest
	// resolution is close to universally supported by real UVC hardware (confirmed on the bench
	// Lenovo camera: MJPG, discrete sizes, 5-30fps). ApplyFormat leaves width/height at whatever
	// the driver's own default was if this particular request fails, rather than failing Open()
	// outright over it.
	ApplyFormat(640, 480, V4L2_PIX_FMT_MJPEG);

	return StartStreaming();
}

void V4l2CameraBackend::Close()
{
	StopStreaming();
	if (m_Fd >= 0) {
		close(m_Fd);
		m_Fd = -1;
	}
}

bool V4l2CameraBackend::IsOpened() const
{
	return m_Fd >= 0;
}

bool V4l2CameraBackend::ApplyFormat(int width, int height, uint32_t fourcc)
{
	if (m_Fd < 0) return false;

	v4l2_format fmt{};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = fourcc;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

	return XIoctl(m_Fd, VIDIOC_S_FMT, &fmt) >= 0;
}

bool V4l2CameraBackend::StartStreaming()
{
	if (m_Streaming) return true;
	if (m_Fd < 0) return false;

	v4l2_requestbuffers req{};
	req.count = BUFFER_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (XIoctl(m_Fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 1) return false;

	m_Buffers.resize(req.count);
	for (unsigned int i = 0; i < req.count; i++) {
		v4l2_buffer buf{};
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (XIoctl(m_Fd, VIDIOC_QUERYBUF, &buf) < 0) {
			m_Buffers.clear();
			return false;
		}

		m_Buffers[i].length = buf.length;
		m_Buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_Fd, buf.m.offset);
		if (m_Buffers[i].start == MAP_FAILED) {
			m_Buffers[i].start = nullptr;
			m_Buffers.clear();
			return false;
		}

		if (XIoctl(m_Fd, VIDIOC_QBUF, &buf) < 0) {
			m_Buffers.clear();
			return false;
		}
	}

	v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (XIoctl(m_Fd, VIDIOC_STREAMON, &type) < 0) {
		m_Buffers.clear();
		return false;
	}

	m_Streaming = true;
	return true;
}

void V4l2CameraBackend::StopStreaming()
{
	if (m_Fd >= 0 && m_Streaming) {
		v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		XIoctl(m_Fd, VIDIOC_STREAMOFF, &type);
	}
	for (MappedBuffer& buffer : m_Buffers) {
		if (buffer.start) munmap(buffer.start, buffer.length);
	}
	m_Buffers.clear();
	m_Streaming = false;

	// REQBUFS(count=0) releases the kernel's own buffer allocation - the V4L2 spec (confirmed the
	// hard way, on the real bench camera: VIDIOC_S_FMT failed with EBUSY every time SetMode() ran
	// after an initial Open()) rejects VIDIOC_S_FMT while ANY buffers are still allocated, even
	// with streaming already off. Only meaningful if m_Fd is still open - StopStreaming() is also
	// called from Close(), by which point the fd may already be gone.
	if (m_Fd >= 0) {
		v4l2_requestbuffers req{};
		req.count = 0;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;
		XIoctl(m_Fd, VIDIOC_REQBUFS, &req);
	}
}

CameraGrabResult V4l2CameraBackend::Grab()
{
	CameraGrabResult result;
	if (m_Fd < 0 || !m_Streaming) return result;

	pollfd pfd{};
	pfd.fd = m_Fd;
	pfd.events = POLLIN;
	int pollResult = poll(&pfd, 1, POLL_TIMEOUT_MS);
	if (pollResult <= 0) return result; // timeout or error - not fatal, just no frame this cycle

	v4l2_buffer buf{};
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (XIoctl(m_Fd, VIDIOC_DQBUF, &buf) < 0) return result;

	// stamped immediately after DQBUF returns - the closest this process gets to "the moment the
	// driver made this frame available", matching OpenCvCameraBackend's own stamp-right-after-read
	// discipline (see its comment; stereo pairing's skew gate depends on this being close to real
	// capture time, not publish time).
	result.captureTimeUs = SourceResult::NowUs();

	v4l2_format currentFmt{};
	currentFmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	XIoctl(m_Fd, VIDIOC_G_FMT, &currentFmt);
	uint32_t fourcc = currentFmt.fmt.pix.pixelformat;

	const uint8_t* data = static_cast<const uint8_t*>(m_Buffers[buf.index].start);
	size_t bytesUsed = buf.bytesused;

	// Every branch below targets a FramePool buffer sized to the negotiated mode's own
	// width/height, CV_8UC3 (BGR - every branch produces BGR regardless of the wire format), and
	// requests it BEFORE the decode/convert/copy call so that call's own Mat::create() fast path
	// (already-right-shape => write in place, no allocation) actually fires. Acquire()ing after
	// the fact - into an already-decoded temporary - would just move the allocation, not remove
	// it. If a real frame ever comes back a different size than the negotiated mode (a malformed
	// JPEG, in practice), Mat::create() falls back to a normal one-off allocation for that frame
	// only - safe, just not pooled that cycle.
	int width = currentFmt.fmt.pix.width, height = currentFmt.fmt.pix.height;
	if (fourcc == V4L2_PIX_FMT_MJPEG || fourcc == V4L2_PIX_FMT_JPEG) {
		cv::Mat jpegView(1, static_cast<int>(bytesUsed), CV_8UC1, const_cast<uint8_t*>(data));
		// imdecode's 3-arg overload reuses *dst in place when it's already the right size/type
		// (its own doc comment: "can save the image reallocations when called repeatedly for
		// images of the same size") - exactly the fast path Acquire()ing at the negotiated mode's
		// own size is meant to hit every frame. Its return value, not the pre-set dst, is the
		// authoritative decoded Mat (a genuine size mismatch reallocates a fresh one instead -
		// result.poolOwner then just outlives an unused buffer harmlessly, see this function's
		// own comment above).
		result.frame = FramePool::Instance().Acquire(height, width, CV_8UC3, result.poolOwner);
		result.frame = cv::imdecode(jpegView, cv::IMREAD_COLOR, &result.frame);
		result.success = !result.frame.empty();
	} else if (fourcc == V4L2_PIX_FMT_YUYV) {
		cv::Mat yuyv(height, width, CV_8UC2, const_cast<uint8_t*>(data));
		result.frame = FramePool::Instance().Acquire(height, width, CV_8UC3, result.poolOwner);
		cv::cvtColor(yuyv, result.frame, cv::COLOR_YUV2BGR_YUYV);
		result.success = true;
	} else if (fourcc == V4L2_PIX_FMT_BGR24) {
		cv::Mat bgr(height, width, CV_8UC3, const_cast<uint8_t*>(data));
		result.frame = FramePool::Instance().Acquire(height, width, CV_8UC3, result.poolOwner);
		bgr.copyTo(result.frame);
		result.success = true;
	} else if (fourcc == V4L2_PIX_FMT_GREY) {
		cv::Mat gray(height, width, CV_8UC1, const_cast<uint8_t*>(data));
		result.frame = FramePool::Instance().Acquire(height, width, CV_8UC3, result.poolOwner);
		cv::cvtColor(gray, result.frame, cv::COLOR_GRAY2BGR);
		result.success = true;
	} else {
		// no decode path for this format (e.g. NV12 straight off the wire) - fail this frame
		// rather than handing back garbage reinterpreted as BGR.
		result.success = false;
	}

	// requeue the same buffer regardless of decode outcome - a bad frame still has to go back to
	// the kernel or the buffer pool starves after BUFFER_COUNT failures.
	XIoctl(m_Fd, VIDIOC_QBUF, &buf);

	return result;
}

std::vector<CameraMode> V4l2CameraBackend::EnumerateModes()
{
	std::vector<CameraMode> modes;
	if (m_Fd < 0) return modes;

	for (unsigned int fmtIndex = 0; ; fmtIndex++) {
		v4l2_fmtdesc fmtDesc{};
		fmtDesc.index = fmtIndex;
		fmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (XIoctl(m_Fd, VIDIOC_ENUM_FMT, &fmtDesc) < 0) break;

		for (unsigned int sizeIndex = 0; ; sizeIndex++) {
			v4l2_frmsizeenum frmSize{};
			frmSize.index = sizeIndex;
			frmSize.pixel_format = fmtDesc.pixelformat;
			if (XIoctl(m_Fd, VIDIOC_ENUM_FRAMESIZES, &frmSize) < 0) break;
			// STEPWISE/CONTINUOUS ranges exist on some drivers but no real UVC webcam this
			// project targets reports them (confirmed on the bench Lenovo camera: discrete only) -
			// skip rather than guess at a representative size from a range.
			if (frmSize.type != V4L2_FRMSIZE_TYPE_DISCRETE) continue;

			for (unsigned int ivalIndex = 0; ; ivalIndex++) {
				v4l2_frmivalenum frmIval{};
				frmIval.index = ivalIndex;
				frmIval.pixel_format = fmtDesc.pixelformat;
				frmIval.width = frmSize.discrete.width;
				frmIval.height = frmSize.discrete.height;
				if (XIoctl(m_Fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmIval) < 0) break;
				if (frmIval.type != V4L2_FRMIVAL_TYPE_DISCRETE) continue;

				CameraMode mode;
				mode.width = frmSize.discrete.width;
				mode.height = frmSize.discrete.height;
				mode.fps = frmIval.discrete.numerator > 0
					? static_cast<double>(frmIval.discrete.denominator) / frmIval.discrete.numerator
					: 0.0;
				mode.pixelFormat = FourCcToFrameFormat(fmtDesc.pixelformat);
				modes.push_back(mode);
			}
		}
	}

	return modes;
}

bool V4l2CameraBackend::SetMode(const CameraMode& mode)
{
	if (m_Fd < 0) return false;

	bool wasStreaming = m_Streaming;
	StopStreaming();

	bool applied = ApplyFormat(mode.width, mode.height, FrameFormatToFourCc(mode.pixelFormat));

	if (mode.fps > 0.0) {
		v4l2_streamparm parm{};
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = static_cast<uint32_t>(mode.fps);
		// not all devices support S_PARM (frame rate control) - failure here doesn't invalidate
		// the resolution/format change ApplyFormat already made.
		XIoctl(m_Fd, VIDIOC_S_PARM, &parm);
	}

	m_RequestedMode = mode;

	if (wasStreaming) return StartStreaming() && applied;
	return applied;
}

CameraMode V4l2CameraBackend::GetCurrentMode() const
{
	CameraMode mode;
	if (m_Fd < 0) return mode;

	v4l2_format fmt{};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (XIoctl(m_Fd, VIDIOC_G_FMT, const_cast<v4l2_format*>(&fmt)) < 0) return mode;

	mode.width = fmt.fmt.pix.width;
	mode.height = fmt.fmt.pix.height;
	mode.pixelFormat = FourCcToFrameFormat(fmt.fmt.pix.pixelformat);

	v4l2_streamparm parm{};
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (XIoctl(m_Fd, VIDIOC_G_PARM, const_cast<v4l2_streamparm*>(&parm)) >= 0 &&
		parm.parm.capture.timeperframe.numerator > 0) {
		mode.fps = static_cast<double>(parm.parm.capture.timeperframe.denominator) /
			parm.parm.capture.timeperframe.numerator;
	}

	if (m_RequestedMode.has_value()) {
		const CameraMode& requested = m_RequestedMode.value();
		mode.isNative = requested.width == mode.width &&
			requested.height == mode.height &&
			requested.pixelFormat == mode.pixelFormat;
	}

	return mode;
}

bool V4l2CameraBackend::SetExposure(int exposureAbsolute)
{
	if (m_Fd < 0) return false;
	v4l2_control ctrl{};
	ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
	ctrl.value = exposureAbsolute;
	return XIoctl(m_Fd, VIDIOC_S_CTRL, &ctrl) >= 0;
}

bool V4l2CameraBackend::SetAutoExposure(bool enabled)
{
	if (m_Fd < 0) return false;
	v4l2_control ctrl{};
	ctrl.id = V4L2_CID_EXPOSURE_AUTO;
	// UVC convention (V4L2_EXPOSURE_APERTURE_PRIORITY / V4L2_EXPOSURE_MANUAL) - most webcams only
	// implement these two of the four standard values.
	ctrl.value = enabled ? V4L2_EXPOSURE_APERTURE_PRIORITY : V4L2_EXPOSURE_MANUAL;
	return XIoctl(m_Fd, VIDIOC_S_CTRL, &ctrl) >= 0;
}

bool V4l2CameraBackend::SetGain(int gain)
{
	if (m_Fd < 0) return false;
	v4l2_control ctrl{};
	ctrl.id = V4L2_CID_GAIN;
	ctrl.value = gain;
	return XIoctl(m_Fd, VIDIOC_S_CTRL, &ctrl) >= 0;
}
