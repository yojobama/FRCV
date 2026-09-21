#include "StereoDepthNode.h"
#include "SgbmStereoBackend.h"
#ifdef LUMEN_WITH_CODEC_STEREO
#include "CodecStereoBackend.h"
#include <codec_stereo/cs_util.h> // cs_shift_gray8 - sign self-check
#endif

#include <algorithm>
#include <cmath>
#include <chrono>
#include <stdexcept>

namespace {
	// codec-stereo's own default (matches lavc_sw's H.264 16x16 macroblock); rkmpp_hwenc's
	// native granularity is forced by hardware to 32x16 regardless of what's requested here -
	// see STEREO_IMPLEMENTATION_PLAN.md ss10.3's "Align dimensions".
	void BlockSizeFor(StereoDepthBackendKind kind, int& blockW, int& blockH)
	{
		if (kind == STEREO_BACKEND_CODEC_RKMPP_HWENC) { blockW = 32; blockH = 16; }
		else { blockW = 16; blockH = 16; }
	}
}

StereoDepthNode::StereoDepthNode(std::shared_ptr<Logger> logger, std::string id,
	StereoDepthBackendKind backend, StereoCalibrationResult calibration,
	double minDepthMeters, double maxDepthMeters,
	int64_t maxSkewUs, StereoFrameOutput frameOutput)
	: ISink(logger, 2, false, true, id), ISource(logger, id),
	  m_Logger(logger), m_BackendKind(backend), m_Calibration(calibration),
	  m_MinDepthMeters(minDepthMeters), m_MaxDepthMeters(maxDepthMeters),
	  m_MaxSkewUs(maxSkewUs), m_FrameOutput(frameOutput)
{
	m_DoNotLoadCaptureThread = true;

	if (minDepthMeters <= 0.0 || maxDepthMeters <= minDepthMeters) {
		throw std::runtime_error("StereoDepthNode: minDepthMeters must be > 0 and < maxDepthMeters");
	}

	int blockW, blockH;
	BlockSizeFor(backend, blockW, blockH);

	// disparity window derivation - see STEREO_IMPLEMENTATION_PLAN.md ss10.3's worked example.
	// Only meaningful once a real calibration is attached; a node created before RunCalibration()
	// has run (fx/baseline both 0) gets a degenerate window and simply won't produce valid
	// blocks until a real StereoCalibrationResult is supplied.
	double fx = calibration.rectifiedFx, baseline = calibration.baselineMeters;
	double dNear = (fx > 0 && baseline > 0) ? fx * baseline / minDepthMeters : 0.0;
	double dFar = (fx > 0 && baseline > 0) ? fx * baseline / maxDepthMeters : 0.0;
	int32_t disparityOffset = (int32_t)std::llround((dNear + dFar) / 2.0);
	int searchRangeX = (int)std::ceil((dNear - dFar) / 2.0) + 8; // + margin
	if (searchRangeX < blockW) searchRangeX = blockW;

	if (backend == STEREO_BACKEND_SGBM) {
		int minDisparity = std::max(0, (int)std::floor(dFar));
		int numDisparities = (int)std::ceil(dNear - minDisparity) + 16;
		m_Backend = std::make_unique<SgbmStereoBackend>(blockW, blockH, minDisparity, numDisparities);
	} else {
#ifdef LUMEN_WITH_CODEC_STEREO
		CodecStereoBackend::Config cfg;
		cfg.kind = backend;
		cfg.blockW = blockW; cfg.blockH = blockH;
		cfg.searchRangeX = searchRangeX; cfg.searchRangeY = 16;
		cfg.disparityOffset = disparityOffset;
		cfg.invertDisparitySign = false; // resolved by RunSignSelfCheckIfNeeded on first real pair
		cfg.minDisparity = (float)dFar;
		cfg.maxDy = 4;
		cfg.maxCost = 0;
		m_Backend = std::make_unique<CodecStereoBackend>(cfg);
#else
		throw std::runtime_error(
			"StereoDepthNode: a codec-stereo backend was requested but LumenVision was built without "
			"LUMEN_WITH_CODEC_STEREO. Use STEREO_BACKEND_SGBM, or rebuild with the flag set - see "
			"STEREO_IMPLEMENTATION_PLAN.md ss10.1.");
#endif
	}
}

StereoDepthNode::~StereoDepthNode() = default;

void StereoDepthNode::SetStereoRoles(const std::string& leftSourceId, const std::string& rightSourceId)
{
	m_LeftSourceId = leftSourceId;
	m_RightSourceId = rightSourceId;
}

std::string StereoDepthNode::GetBackendName() const
{
	return m_Backend ? m_Backend->Name() : "none";
}

double StereoDepthNode::GetLastValidFraction() const
{
	std::lock_guard<std::mutex> lock(m_StatsMutex);
	return m_LastValidFraction;
}

double StereoDepthNode::GetLastMedianDepthMeters() const
{
	std::lock_guard<std::mutex> lock(m_StatsMutex);
	return m_LastMedianDepthMeters;
}

bool StereoDepthNode::GetLastDepthGrid(std::vector<float>& outDepth, int& cols, int& rows, int& blockW, int& blockH) const
{
	std::lock_guard<std::mutex> lock(m_StatsMutex);
	if (!m_HasDepthGrid) return false;
	outDepth = m_LastDepthGrid;
	cols = m_LastCols;
	rows = m_LastRows;
	blockW = m_Backend->BlockW();
	blockH = m_Backend->BlockH();
	return true;
}

void StereoDepthNode::EnsureRectifyMaps(const cv::Size& sourceSize)
{
	if (sourceSize == m_RectifiedSourceSize && !m_MapLx.empty()) return;

	if (sourceSize.width != m_Calibration.imageWidth || sourceSize.height != m_Calibration.imageHeight) {
		throw std::runtime_error(
			"StereoDepthNode: source frame is " + std::to_string(sourceSize.width) + "x" + std::to_string(sourceSize.height) +
			" but the attached calibration is only valid at " + std::to_string(m_Calibration.imageWidth) + "x" +
			std::to_string(m_Calibration.imageHeight) + " - a calibration is only valid at the exact resolution it was computed at.");
	}

	auto vecToK = [](const CameraCalibrationResult& c) {
		cv::Mat k = cv::Mat::eye(3, 3, CV_64F);
		k.at<double>(0, 0) = c.fx; k.at<double>(1, 1) = c.fy;
		k.at<double>(0, 2) = c.cx; k.at<double>(1, 2) = c.cy;
		return k;
	};
	auto vecToD = [](const CameraCalibrationResult& c) {
		return cv::Mat(c.distCoeffs, true).reshape(1, (int)c.distCoeffs.size());
	};
	auto vec9ToMat = [](const std::vector<double>& v) {
		cv::Mat m(3, 3, CV_64F);
		for (int i = 0; i < 9; i++) m.at<double>(i / 3, i % 3) = v[i];
		return m;
	};
	auto vec12ToMat = [](const std::vector<double>& v) {
		cv::Mat m(3, 4, CV_64F);
		for (int i = 0; i < 12; i++) m.at<double>(i / 4, i % 4) = v[i];
		return m;
	};

	cv::Mat K1 = vecToK(m_Calibration.left), D1 = vecToD(m_Calibration.left);
	cv::Mat K2 = vecToK(m_Calibration.right), D2 = vecToD(m_Calibration.right);
	cv::Mat R1 = vec9ToMat(m_Calibration.R1), R2 = vec9ToMat(m_Calibration.R2);
	cv::Mat P1 = vec12ToMat(m_Calibration.P1), P2 = vec12ToMat(m_Calibration.P2);

	// CV_16SC2 fixed-point maps: the fast path per ss10.3 - float maps cost meaningfully more
	// for no accuracy that matters to a block-granular disparity backend.
	cv::initUndistortRectifyMap(K1, D1, R1, P1, sourceSize, CV_16SC2, m_MapLx, m_MapLy);
	cv::initUndistortRectifyMap(K2, D2, R2, P2, sourceSize, CV_16SC2, m_MapRx, m_MapRy);
	m_RectifiedSourceSize = sourceSize;

	// crop to the intersection of both eyes' valid ROIs, then round down to the backend's
	// native block size - never pad with black (a synthetic border is a huge, perfectly
	// matchable feature that pulls motion vectors toward it). See ss10.3.
	cv::Rect roiL(m_Calibration.roiLeftX, m_Calibration.roiLeftY, m_Calibration.roiLeftW, m_Calibration.roiLeftH);
	cv::Rect roiR(m_Calibration.roiRightX, m_Calibration.roiRightY, m_Calibration.roiRightW, m_Calibration.roiRightH);
	cv::Rect roi = (roiL.width > 0 && roiR.width > 0) ? (roiL & roiR) : cv::Rect(0, 0, sourceSize.width, sourceSize.height);
	if (roi.width <= 0 || roi.height <= 0) roi = cv::Rect(0, 0, sourceSize.width, sourceSize.height);

	int blockW = m_Backend->BlockW(), blockH = m_Backend->BlockH();
	m_CropW = (roi.width / blockW) * blockW;
	m_CropH = (roi.height / blockH) * blockH;
	if (m_CropW <= 0 || m_CropH <= 0) {
		throw std::runtime_error("StereoDepthNode: valid rectified ROI is smaller than one backend block - check calibration quality");
	}
}

void StereoDepthNode::RunSignSelfCheckIfNeeded(const cv::Mat& rectLeftGray)
{
#ifdef LUMEN_WITH_CODEC_STEREO
	if (m_SignCheckDone || m_BackendKind == STEREO_BACKEND_SGBM) { m_SignCheckDone = true; return; }

	// synthesize a known 16px shift (left point at x visible in "right" at x+16, matching this
	// project's own sign convention - see cs.h's cs_mv_field comment) and check the backend
	// resolves it as a positive disparity of the expected magnitude - the codec-stereo Middlebury
	// harness itself got this backwards on its first attempt. Never knowable a priori per the
	// library's own docs, so this is checked on real hardware/optics, not assumed.
	cv::Mat shifted(rectLeftGray.size(), CV_8UC1);
	cs_shift_gray8(rectLeftGray.data, (int)rectLeftGray.step, shifted.data, (int)shifted.step,
		rectLeftGray.cols, rectLeftGray.rows, 16);

	std::vector<float> disp;
	int cols, rows;
	if (m_Backend->Compute(rectLeftGray, shifted, disp, cols, rows)) {
		double sum = 0.0; int n = 0;
		for (float d : disp) { if (d != STEREO_DISPARITY_INVALID) { sum += d; n++; } }
		double mean = n > 0 ? sum / n : 0.0;
		m_InvertDisparitySign = mean < 0.0;
		if (m_Logger) m_Logger->EnterLog("StereoDepthNode: sign self-check recovered mean disparity " +
			std::to_string(mean) + " for a known +16px shift -> invertDisparitySign=" + std::to_string(m_InvertDisparitySign));
	}
	m_SignCheckDone = true;
#else
	m_SignCheckDone = true;
#endif
}

void StereoDepthNode::Process(std::vector<SourceResult> results)
{
	for (auto& r : results) {
		if (r.sourceId == m_LeftSourceId) m_PendingLeft = r;
		else if (r.sourceId == m_RightSourceId) m_PendingRight = r;
	}
	if (!m_PendingLeft.has_value() || !m_PendingRight.has_value()) return;

	int64_t skewUs = std::llabs((int64_t)m_PendingLeft->captureTimeUs - (int64_t)m_PendingRight->captureTimeUs);
	if (skewUs > m_MaxSkewUs) {
		if (m_PendingLeft->captureTimeUs < m_PendingRight->captureTimeUs) m_PendingLeft.reset();
		else m_PendingRight.reset();
		return;
	}

	SourceResult left = *m_PendingLeft, right = *m_PendingRight;
	m_PendingLeft.reset();
	m_PendingRight.reset();

	if (!left.frame.has_value() || !right.frame.has_value() || left.frame->empty() || right.frame->empty()) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "StereoDepthNode: blank frame in a paired stereo result.");
		return;
	}
	if (!m_Calibration.IsValid()) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "StereoDepthNode: no valid calibration attached yet.");
		return;
	}

	auto t0 = std::chrono::steady_clock::now();

	try {
		EnsureRectifyMaps(left.frame->size());
	} catch (const std::exception& e) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, std::string("StereoDepthNode: ") + e.what());
		return;
	}

	// gray first, THEN remap - remapping one channel instead of three is a straight 3x saving on
	// the most expensive fixed cost in this path, and codec-stereo/SGBM only need luma anyway.
	cv::Mat grayLeft, grayRight, rectLeft, rectRight;
	cv::cvtColor(*left.frame, grayLeft, cv::COLOR_BGR2GRAY);
	cv::cvtColor(*right.frame, grayRight, cv::COLOR_BGR2GRAY);
	cv::remap(grayLeft, rectLeft, m_MapLx, m_MapLy, cv::INTER_LINEAR);
	cv::remap(grayRight, rectRight, m_MapRx, m_MapRy, cv::INTER_LINEAR);

	cv::Mat cropLeft = rectLeft(cv::Rect(0, 0, m_CropW, m_CropH));
	cv::Mat cropRight = rectRight(cv::Rect(0, 0, m_CropW, m_CropH));

	auto t1 = std::chrono::steady_clock::now();

	RunSignSelfCheckIfNeeded(cropLeft);

	std::vector<float> disparity;
	int cols = 0, rows = 0;
	bool ok = m_Backend->Compute(cropLeft, cropRight, disparity, cols, rows);

	auto t2 = std::chrono::steady_clock::now();
	double rectifyMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
	double extractMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

	if (!ok) {
		if (m_Logger) m_Logger->EnterLog(LogLevel::Error, "StereoDepthNode: backend Compute() failed.");
		return;
	}

	double fx = m_Calibration.rectifiedFx, baseline = m_Calibration.baselineMeters;
	std::vector<float> depth(disparity.size(), 0.0f);
	std::vector<float> validDepths;
	validDepths.reserve(disparity.size());
	for (size_t i = 0; i < disparity.size(); i++) {
		float d = disparity[i];
		if (m_InvertDisparitySign && d != STEREO_DISPARITY_INVALID) d = -d;
		if (d == STEREO_DISPARITY_INVALID || d <= 0.0f) continue;
		float z = (float)(fx * baseline / d);
		depth[i] = z;
		validDepths.push_back(z);
	}

	double validFraction = disparity.empty() ? 0.0 : (double)validDepths.size() / disparity.size();
	double medianDepth = 0.0;
	if (!validDepths.empty()) {
		std::vector<float> sorted = validDepths;
		std::nth_element(sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end());
		medianDepth = sorted[sorted.size() / 2];
	}

	{
		std::lock_guard<std::mutex> lock(m_StatsMutex);
		m_LastValidFraction = validFraction;
		m_LastMedianDepthMeters = medianDepth;
		m_LastDepthGrid = depth;
		m_LastCols = cols;
		m_LastRows = rows;
		m_HasDepthGrid = true;
	}

	// output frame - see StereoFrameOutput.h / ss10.3 "Outputs"
	cv::Mat outFrame;
	if (m_FrameOutput == STEREO_FRAME_RECTIFIED_LEFT) {
		cv::Mat colorRectLeft;
		cv::remap(*left.frame, colorRectLeft, m_MapLx, m_MapLy, cv::INTER_LINEAR);
		outFrame = colorRectLeft(cv::Rect(0, 0, m_CropW, m_CropH)).clone();
	} else {
		cv::Mat depthGrid(rows, cols, CV_32F, depth.data());
		cv::Mat normalized;
		double range = m_MaxDepthMeters - m_MinDepthMeters;
		depthGrid.convertTo(normalized, CV_8U, range > 0 ? 255.0 / range : 1.0, range > 0 ? -255.0 * m_MinDepthMeters / range : 0.0);
		cv::Mat colormap;
		cv::applyColorMap(normalized, colormap, cv::COLORMAP_TURBO);
		// invalid cells (depth==0) rendered black rather than a misleadingly "near" color
		for (int by = 0; by < rows; by++)
			for (int bx = 0; bx < cols; bx++)
				if (depth[(size_t)by * cols + bx] <= 0.0f) colormap.at<cv::Vec3b>(by, bx) = cv::Vec3b(0, 0, 0);

		cv::Mat upscaled;
		cv::resize(colormap, upscaled, cv::Size(m_CropW, m_CropH), 0, 0, cv::INTER_NEAREST);

		if (m_FrameOutput == STEREO_FRAME_DEPTH_OVERLAY) {
			cv::Mat colorRectLeft;
			cv::remap(*left.frame, colorRectLeft, m_MapLx, m_MapLy, cv::INTER_LINEAR);
			cv::Mat cropped = colorRectLeft(cv::Rect(0, 0, m_CropW, m_CropH));
			cv::addWeighted(cropped, 0.5, upscaled, 0.5, 0.0, outFrame);
		} else {
			outFrame = upscaled;
		}
	}

	nlohmann::json json = {
		{"backend", GetBackendName()}, {"cols", cols}, {"rows", rows},
		{"blockW", m_Backend->BlockW()}, {"blockH", m_Backend->BlockH()},
		{"validFraction", validFraction}, {"medianDepthM", medianDepth},
		{"rectifyMs", rectifyMs}, {"extractMs", extractMs}, {"skewUs", skewUs},
	};

	SetLatestResult(SourceResult(json, outFrame));
}
