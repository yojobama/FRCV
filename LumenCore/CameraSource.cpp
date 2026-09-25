#include "CameraSource.h"
#include "Frame.h"
#include "OpenCvCameraBackend.h"
#ifdef __linux__
#include "V4l2CameraBackend.h"
#endif

namespace {
	// AUTO backend selection: V4L2 on Linux (real mode/exposure control, DMA-friendly capture),
	// falling back to OpenCvCameraBackend if the device won't open through V4L2 at all (e.g. a
	// non-UVC capture device V4l2CameraBackend's QUERYCAP capability check rejects) - the same
	// construction-failure-falls-back-to-CPU pattern ApriltagDetector.cpp already uses for its
	// Vulkan->CPU fallback. OpenCvCameraBackend is the only option on every other platform until
	// MediaFoundationCameraBackend (ROADMAP.md Phase 3) lands.
	std::unique_ptr<ICameraBackend> OpenAutoBackend(const std::string& devicePath, std::shared_ptr<Logger> logger)
	{
#ifdef __linux__
		auto v4l2Backend = std::make_unique<V4l2CameraBackend>();
		if (v4l2Backend->Open(devicePath)) return v4l2Backend;
		if (logger) logger->EnterLog(LogLevel::Warning, "V4L2 backend failed to open " + devicePath + ", falling back to OpenCV");
#endif
		auto openCvBackend = std::make_unique<OpenCvCameraBackend>();
		openCvBackend->Open(devicePath);
		return openCvBackend;
	}
}

CameraFrameSource::CameraFrameSource(std::string devicePath, std::shared_ptr<Logger> logger, std::string id) : ISource(logger, id)
{
    m_Backend = OpenAutoBackend(devicePath, logger);
    if (!m_Backend->IsOpened()) throw "unable to open webcam: " + devicePath;

    this->m_DevicePath = devicePath;
    this->m_DeviceName = getDeviceName();
}

CameraFrameSource::CameraFrameSource(std::string devicePath, std::string deviceName, std::shared_ptr<Logger> logger, std::string id) : ISource(logger, id)
{
    m_Backend = OpenAutoBackend(devicePath, logger);
    if (!m_Backend->IsOpened()) {
        // previously silent: this constructor never checked whether the device actually opened,
        // so a persisted-but-now-missing camera produced no frames forever with no diagnostic.
        if (logger) logger->EnterLog(LogLevel::Error, "unable to open webcam: " + devicePath);
    }
    this->m_DeviceName = deviceName;
    this->m_DevicePath = devicePath;
}

CameraFrameSource::~CameraFrameSource()
{
}

std::string CameraFrameSource::getDevicePath()
{
    return m_DevicePath;
}

std::string CameraFrameSource::getDeviceName()
{
    return m_DeviceName;
}

void CameraFrameSource::changeDeviceName(std::string newName)
{
    this->m_DeviceName = newName;
}

std::vector<CameraMode> CameraFrameSource::GetAvailableModes()
{
    return m_Backend->EnumerateModes();
}

CameraMode CameraFrameSource::GetCurrentMode()
{
    return m_Backend->GetCurrentMode();
}

bool CameraFrameSource::SetMode(const CameraMode& mode)
{
    return m_Backend->SetMode(mode);
}

bool CameraFrameSource::SetExposure(int exposureAbsolute)
{
    return m_Backend->SetExposure(exposureAbsolute);
}

bool CameraFrameSource::SetAutoExposure(bool enabled)
{
    return m_Backend->SetAutoExposure(enabled);
}

bool CameraFrameSource::SetGain(int gain)
{
    return m_Backend->SetGain(gain);
}

CameraControlRange CameraFrameSource::GetExposureRange()
{
    return m_Backend->GetExposureRange();
}

CameraControlRange CameraFrameSource::GetGainRange()
{
    return m_Backend->GetGainRange();
}

void CameraFrameSource::CaptureFrame()
{
    if (m_Backend->IsOpened()) {
        CameraGrabResult grab = m_Backend->Grab();
        if (grab.success) {
            // Carries grab.poolOwner through explicitly (not the implicit bare-cv::Mat
            // conversion SourceResult also accepts) - THAT overload has no pool-owner parameter
            // at all, so going through it here would silently let the FramePool buffer get
            // recycled the moment this function returns, out from under every sink still
            // processing this exact frame on its own thread. See ICameraBackend.h's own comment
            // on CameraGrabResult::poolOwner.
            SetLatestResult(SourceResult(std::nullopt, Frame(grab.frame, grab.format, grab.poolOwner), grab.captureTimeUs));
        } else {
            // previously: this branch didn't exist at all - a failed grab (device still open,
            // read() returning false) was silently dropped with no diagnostic.
            m_Logger->EnterLog(LogLevel::Error, "camera grab failed for " + m_DevicePath);
        }
    } else {
        // previously: logged unconditionally after the if-block, so a SUCCESSFUL grab logged
        // "camera is closed" on every single frame - see ROADMAP.md Phase 6's latent-bugs list.
        m_Logger->EnterLog(LogLevel::Error, "camera is closed, not capturing a frame");
    }
}
