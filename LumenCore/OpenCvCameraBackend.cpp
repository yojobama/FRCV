#include "OpenCvCameraBackend.h"
#include "SourceResult.h"

bool OpenCvCameraBackend::Open(const std::string& devicePath)
{
#ifdef __linux__
	m_Capture = cv::VideoCapture(devicePath, cv::CAP_V4L2);
#else
	// cv::VideoCapture's string-path constructor only resolves a numeric index (or an actual
	// video FILE path) on Windows, not a device symbolic link - real device-path opening on
	// Windows is MediaFoundationCameraBackend's job (ROADMAP.md Phase 3); this fallback exists
	// for parity/testing there, not as the real Windows camera path.
	m_Capture = cv::VideoCapture(devicePath);
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
