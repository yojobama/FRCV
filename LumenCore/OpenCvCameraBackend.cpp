#include "OpenCvCameraBackend.h"
#include "SourceResult.h"
#include <cctype>
#include <algorithm>

bool OpenCvCameraBackend::Open(const std::string& devicePath)
{
#ifdef __linux__
	m_Capture = cv::VideoCapture(devicePath, cv::CAP_V4L2);
#else
	// A real device symbolic link (Windows has no V4L2 equivalent) is still
	// MediaFoundationCameraBackend's job (ROADMAP.md Phase 3, not yet written), but a plain
	// numeric index - what EnumerateAvailableCameras' own Windows stub would hand back if/when
	// it's implemented, and what a caller passes directly in the meantime (confirmed the hard
	// way: cv::VideoCapture's STRING constructor does NOT reliably resolve a numeric string to
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
		m_Capture = cv::VideoCapture(index, cv::CAP_MSMF);
		if (!m_Capture.isOpened()) {
			// some devices (confirmed against a real Windows Hello IR+RGB combo camera) simply
			// don't open via Media Foundation at all despite Device Manager reporting them
			// healthy - legacy DirectShow is the fallback, not a third guess: it's the other
			// backend OpenCV's own Windows build actually ships, and cv::VideoCapture's own
			// generic (no-backend-specified) constructor already tries both internally in some
			// order, so being explicit about the fallback here is strictly more informative than
			// letting that internal order decide silently.
			m_Capture = cv::VideoCapture(index, cv::CAP_DSHOW);
		}
		if (!m_Capture.isOpened()) {
			m_Capture = cv::VideoCapture(index, cv::CAP_ANY);
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
