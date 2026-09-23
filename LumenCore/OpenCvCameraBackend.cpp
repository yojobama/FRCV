#include "OpenCvCameraBackend.h"
#include "SourceResult.h"
#include <cctype>
#include <algorithm>
#include <chrono>
#include <future>
#include <thread>

namespace {
	// cv::VideoCapture's own open call is a plain blocking OS/driver call with no cancellation
	// and no timeout of its own - confirmed a real, uncooperative device (Device Manager reports
	// it healthy, but the open call itself never returns) can hang here forever, which is fatal
	// at server startup: DB.Load() reconstructs every persisted camera source SYNCHRONOUSLY, so
	// one bad entry in data.json was enough to make the whole server unable to start at all, with
	// no way to fix it short of hand-editing that file. Runs the actual open on a detached
	// worker thread and gives up waiting after `timeout` - if the open call really is stuck
	// forever, that one thread (and whatever device handle it's holding) just leaks for the rest
	// of the process's life instead of blocking startup, which is a real but far smaller cost.
	cv::VideoCapture OpenWithTimeout(int index, int backend, std::chrono::milliseconds timeout)
	{
		auto promise = std::make_shared<std::promise<cv::VideoCapture>>();
		std::future<cv::VideoCapture> future = promise->get_future();

		std::thread([index, backend, promise]() {
			promise->set_value(cv::VideoCapture(index, backend));
		}).detach();

		if (future.wait_for(timeout) == std::future_status::ready) {
			return future.get();
		}
		return cv::VideoCapture(); // isOpened() == false
	}
}

bool OpenCvCameraBackend::Open(const std::string& devicePath)
{
#ifdef __linux__
	m_Capture = cv::VideoCapture(devicePath, cv::CAP_V4L2);
#else
	// A real device symbolic link (Windows has no V4L2 equivalent) is still a possible future
	// enhancement, but a plain numeric index - what Manager::EnumerateAvailableCameras' Windows
	// implementation (WindowsCameraEnumerator.cpp, Media Foundation's MFEnumDeviceSources) hands
	// back as each device's path - needs the real int overload (confirmed the hard way:
	// cv::VideoCapture's STRING constructor does NOT reliably resolve a numeric string to
	// a device index on Windows the way the dedicated int-index overload does, even though
	// nothing in cv::VideoCapture's own documented behavior rules it out - it simply opened
	// nothing, silently, with isOpened() false and no diagnostic) needs the real int overload,
	// with cv::CAP_MSMF explicitly requested rather than left to OpenCV's own auto-detection -
	// Media Foundation is the modern, actively-maintained Windows capture backend (unlike the
	// legacy DirectShow one), and being explicit here removes one more variable from "why didn't
	// this open" the next time this code runs on a machine this wasn't tested on.
	std::string trimmed = devicePath;
	trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), [](unsigned char c) { return std::isspace(c); }), trimmed.end());
	bool isNumericIndex = !trimmed.empty() && std::all_of(trimmed.begin(), trimmed.end(), [](unsigned char c) { return std::isdigit(c); });
	if (isNumericIndex) {
		int index = std::stoi(trimmed);
		// 5s per backend attempt - generous for a real device (which typically opens in well
		// under a second) without letting one bad camera hold up server startup for long; three
		// attempts worst-case is 15s, not the infinite hang this replaced.
		constexpr auto kOpenTimeout = std::chrono::seconds(5);
		m_Capture = OpenWithTimeout(index, cv::CAP_MSMF, kOpenTimeout);
		if (!m_Capture.isOpened()) {
			// MSMF opening the wrong (or a genuinely restricted) device isn't ruled out just
			// because it worked for one specific camera this session - earlier failures against
			// this same physical machine turned out to be caused by guessing device indices with
			// no real enumerator, not a hardware/driver wall (fixed by the enumerator existing
			// at all - see WindowsCameraEnumerator.cpp), but that doesn't mean every device will
			// open via MSMF. Legacy DirectShow is the fallback, not a third guess: it's the other
			// backend OpenCV's own Windows build actually ships, and cv::VideoCapture's own
			// generic (no-backend-specified) constructor already tries both internally in some
			// order, so being explicit about the fallback here is strictly more informative than
			// letting that internal order decide silently.
			m_Capture = OpenWithTimeout(index, cv::CAP_DSHOW, kOpenTimeout);
		}
		if (!m_Capture.isOpened()) {
			m_Capture = OpenWithTimeout(index, cv::CAP_ANY, kOpenTimeout);
		}
	} else {
		m_Capture = cv::VideoCapture(devicePath);
	}
#endif
	return m_Capture.isOpened();
}

void OpenCvCameraBackend::Close()
{
	if (m_Capture.isOpened()) m_Capture.release();
}

bool OpenCvCameraBackend::IsOpened() const
{
	return m_Capture.isOpened();
}

CameraGrabResult OpenCvCameraBackend::Grab()
{
	CameraGrabResult result;
	if (!m_Capture.isOpened()) return result;
	result.success = m_Capture.read(result.frame);
	// stamped the instant the read returns, not when SetLatestResult later publishes it - the
	// two can be an arbitrary amount of time apart once the sink processing thread is busy, and
	// stereo pairing needs the real capture instant to gate on skew correctly.
	result.captureTimeUs = SourceResult::NowUs();
	return result;
}

std::vector<CameraMode> OpenCvCameraBackend::EnumerateModes()
{
	return {};
}

bool OpenCvCameraBackend::SetMode(const CameraMode& mode)
{
	if (!m_Capture.isOpened()) return false;
	// order matters: some backends (MSMF in particular) reject a resolution change once FPS is
	// already set to an incompatible value for the new size, so set FPS last against the
	// now-current resolution.
	bool ok = m_Capture.set(cv::CAP_PROP_FRAME_WIDTH, mode.width);
	ok = m_Capture.set(cv::CAP_PROP_FRAME_HEIGHT, mode.height) && ok;
	if (mode.fps > 0.0) ok = m_Capture.set(cv::CAP_PROP_FPS, mode.fps) && ok;
	return ok;
}

CameraMode OpenCvCameraBackend::GetCurrentMode() const
{
	CameraMode mode;
	if (!m_Capture.isOpened()) return mode;
	mode.width = static_cast<int>(m_Capture.get(cv::CAP_PROP_FRAME_WIDTH));
	mode.height = static_cast<int>(m_Capture.get(cv::CAP_PROP_FRAME_HEIGHT));
	mode.fps = m_Capture.get(cv::CAP_PROP_FPS);
	// cv::VideoCapture always hands back a decoded BGR cv::Mat regardless of the device's real
	// wire format - reporting BGR24 here is honest about what GetCurrentMode() actually reflects
	// for this backend, not a claim about the device's native capture format.
	mode.pixelFormat = FrameFormat::BGR24;
	return mode;
}

bool OpenCvCameraBackend::SetExposure(int exposureAbsolute)
{
	if (!m_Capture.isOpened()) return false;
	return m_Capture.set(cv::CAP_PROP_EXPOSURE, exposureAbsolute);
}

bool OpenCvCameraBackend::SetAutoExposure(bool enabled)
{
	if (!m_Capture.isOpened()) return false;
	// V4L2 backend convention (also honoured by OpenCV's own V4L2 capture backend on Linux):
	// 3 = aperture priority (auto), 1 = manual. Windows backends (MSMF/DSHOW) largely ignore this
	// property outright - SetExposure's own success/failure is the more reliable signal there.
	return m_Capture.set(cv::CAP_PROP_AUTO_EXPOSURE, enabled ? 3 : 1);
}

bool OpenCvCameraBackend::SetGain(int gain)
{
	if (!m_Capture.isOpened()) return false;
	return m_Capture.set(cv::CAP_PROP_GAIN, gain);
}
