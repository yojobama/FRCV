#pragma once
#include "ISource.h"
#include "ISink.h"
#include "IStereoRoleReceiver.h"
#include "IStereoDepthBackend.h"
#include "StereoCalibrationResult.h"
#include "StereoDepthBackendKind.h"
#include "StereoFrameOutput.h"

#include <memory>
#include <optional>
#include <mutex>

// ISource+ISink, maxSources=2 (left/right cameras, or any pair of frame-producing nodes -
// StereoDepthNode doesn't care whether "left"/"right" are raw cameras or something upstream of
// them). Rectifies each paired frame with maps built once from a StereoCalibrationResult, hands
// the rectified pair to an IStereoDepthBackend, and converts the resulting block-grid disparity
// to depth. See STEREO_IMPLEMENTATION_PLAN.md ss10.3 for the full design rationale (disparity
// window derivation, sign-convention self-check, why cropping beats padding, etc.) - the
// comments below note only the specific "why", not a restatement of that document.
class StereoDepthNode : public ISink, public ISource, public IStereoRoleReceiver
{
public:
	StereoDepthNode(std::shared_ptr<Logger> logger, std::string id,
		StereoDepthBackendKind backend, StereoCalibrationResult calibration,
		double minDepthMeters, double maxDepthMeters,
		int64_t maxSkewUs, StereoFrameOutput frameOutput);
	~StereoDepthNode();

	// IStereoRoleReceiver
	void SetStereoRoles(const std::string& leftSourceId, const std::string& rightSourceId) override;

	std::string GetBackendName() const;

	// summary stats from the most recently processed pair - cols*rows floats is far too much to
	// push through NT4/REST every frame (1080p/16x16 is 8160 floats), so this is what
	// GetSinkResult's JSON actually carries; see ss10.3 "Outputs".
	double GetLastValidFraction() const;
	double GetLastMedianDepthMeters() const;

private:
	void Process(std::vector<SourceResult> results) override;
	void EnsureRectifyMaps(const cv::Size& sourceSize);
	void EnsureBackend(int croppedW, int croppedH);
	// synthesizes a known 16px shift from a real captured frame and checks which sign the
	// backend recovers it as - see ss10.3 "Sign convention". Runs once, on the first successful
	// pair; logs which way it resolved rather than silently trusting the config default.
	void RunSignSelfCheckIfNeeded(const cv::Mat& rectLeftGray);

	std::shared_ptr<Logger> m_Logger;
	StereoDepthBackendKind m_BackendKind;
	StereoCalibrationResult m_Calibration;
	double m_MinDepthMeters, m_MaxDepthMeters;
	int64_t m_MaxSkewUs;
	StereoFrameOutput m_FrameOutput;

	std::string m_LeftSourceId, m_RightSourceId;
	std::optional<SourceResult> m_PendingLeft, m_PendingRight;

	// rectification maps, built once (or rebuilt if the source resolution changes - a
	// calibration is only valid at the exact resolution it was computed at)
	cv::Mat m_MapLx, m_MapLy, m_MapRx, m_MapRy;
	cv::Size m_RectifiedSourceSize;
	int m_CropW = 0, m_CropH = 0;

	std::unique_ptr<IStereoDepthBackend> m_Backend;
	bool m_SignCheckDone = false;
	bool m_InvertDisparitySign = false;

	mutable std::mutex m_StatsMutex;
	double m_LastValidFraction = 0.0;
	double m_LastMedianDepthMeters = 0.0;
};
