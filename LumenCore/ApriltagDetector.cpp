#include "ApriltagDetector.h"
#include "ApriltagDetection.h"
#include "CameraCalibrationResult.h"
#include "CpuApriltagBackend.h"
#include "FramePool.h"
#ifdef LUMEN_WITH_VULKAN_APRILTAG
#include "VkApriltagBackend.h"
#endif

ApriltagDetector::ApriltagDetector(std::shared_ptr<Logger> logger, std::string id, CameraCalibrationResult cameraCalibrationResult,
	double tagSize, ApriltagBackendKind backendKind, int frameWidth, int frameHeight, int nthreads, float quadDecimate)
	: ISource(logger, id), ISink(logger, 1, false, true, id)
{
	if (logger) logger->EnterLog("ApriltagDetector constructed");

	if (backendKind == APRILTAG_BACKEND_VULKAN) {
#ifdef LUMEN_WITH_VULKAN_APRILTAG
		try {
			m_Backend = std::make_unique<VkApriltagBackend>(frameWidth, frameHeight, nthreads);
			m_ActiveBackendKind = APRILTAG_BACKEND_VULKAN;
		} catch (const std::exception& e) {
			// no usable Vulkan compute device, or GpuDetector/pipeline setup failed - fall back
			// to CPU rather than fail to construct at all (plan phase 5, item 5)
			if (logger) logger->EnterLog(LogLevel::Warning,
				std::string("Vulkan AprilTag backend unavailable (") + e.what() + "), falling back to CPU");
			m_Backend = std::make_unique<CpuApriltagBackend>(nthreads, quadDecimate);
			m_ActiveBackendKind = APRILTAG_BACKEND_CPU;
		}
#else
		if (logger) logger->EnterLog(LogLevel::Warning,
			"Vulkan AprilTag backend requested but LUMEN_WITH_VULKAN_APRILTAG was not compiled in, falling back to CPU");
		m_Backend = std::make_unique<CpuApriltagBackend>(nthreads, quadDecimate);
		m_ActiveBackendKind = APRILTAG_BACKEND_CPU;
#endif
	} else {
		m_Backend = std::make_unique<CpuApriltagBackend>(nthreads, quadDecimate);
		m_ActiveBackendKind = APRILTAG_BACKEND_CPU;
	}

	m_OriginalCalibration = cameraCalibrationResult;
	m_DetectionInfo.tagsize = tagSize;
	m_DetectionInfo.fx = cameraCalibrationResult.fx;
	m_DetectionInfo.fy = cameraCalibrationResult.fy;
	m_DetectionInfo.cx = cameraCalibrationResult.cx;
	m_DetectionInfo.cy = cameraCalibrationResult.cy;
	m_HasCalibration = cameraCalibrationResult.fx > 0.0 && cameraCalibrationResult.fy > 0.0;

	// m_CameraMatrix must exist whenever m_HasCalibration does - multi-tag PnP (ROADMAP.md
	// Phase 7) needs real intrinsics regardless of whether this particular calibration happened
	// to fit non-zero distortion coefficients. Previously this was only built inside the
	// HasDistortion() branch, leaving m_CameraMatrix empty for an otherwise-valid calibration
	// with zero-fit distortion - a real gap, not just an edge case multi-tag PnP happened to
	// need fixed anyway.
	if (m_HasCalibration) {
		m_CameraMatrix = (cv::Mat_<double>(3, 3) <<
			cameraCalibrationResult.fx, 0, cameraCalibrationResult.cx,
			0, cameraCalibrationResult.fy, cameraCalibrationResult.cy,
			0, 0, 1);
	}
	if (cameraCalibrationResult.HasDistortion()) {
		m_HasDistortion = true;
		m_DistCoeffs = cv::Mat(cameraCalibrationResult.distCoeffs, true /* copy */);
	} else {
		m_DistCoeffs = cv::Mat::zeros(5, 1, CV_64F);
	}

	m_Logger = logger;

	m_DoNotLoadCaptureThread = true;
}

ApriltagDetector::~ApriltagDetector() = default;

CameraCalibrationResult ApriltagDetector::GetCalibration() const
{
	return m_OriginalCalibration;
}

std::string ApriltagDetector::GetBackendName() const
{
	return m_Backend->Name();
}

nlohmann::json ApriltagDetector::SolveMultiTagPnP(
	const std::vector<cv::Point3d>& objectPoints, const std::vector<cv::Point2d>& imagePoints,
	const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, int tagCount)
{
	// requires at least 2 tags - a single tag's own correspondences alone are exactly what the
	// per-tag estimate_tag_pose path already computes; solvePnP over one tag's 4 (coplanar)
	// corners would just reproduce the same estimate, not a more robust one, for the cost of a
	// second pose solve.
	if (tagCount < 2) return nullptr;

	cv::Mat rvec, tvec;
	bool solved = cv::solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec);
	if (!solved) return nullptr;

	cv::Mat fieldToCameraRotation;
	cv::Rodrigues(rvec, fieldToCameraRotation);

	// solvePnP's own (rvec, tvec) map FIELD points INTO camera frame (p_camera = R*p_field + t)
	// - the camera's own pose IN FIELD frame (what a robot program actually wants: "where am I")
	// is the inverse of that.
	cv::Mat cameraRotationInField = fieldToCameraRotation.t();
	cv::Mat cameraTranslationInField = -cameraRotationInField * tvec;

	std::vector<cv::Point2d> reprojected;
	cv::projectPoints(objectPoints, rvec, tvec, cameraMatrix, distCoeffs, reprojected);
	double squaredErrorSum = 0.0;
	for (size_t p = 0; p < reprojected.size(); p++) {
		double dx = reprojected[p].x - imagePoints[p].x;
		double dy = reprojected[p].y - imagePoints[p].y;
		squaredErrorSum += dx * dx + dy * dy;
	}
	double reprojectionErrorPixels = std::sqrt(squaredErrorSum / reprojected.size());

	return {
		{"x", cameraTranslationInField.at<double>(0)},
		{"y", cameraTranslationInField.at<double>(1)},
		{"z", cameraTranslationInField.at<double>(2)},
		{"R", {
			{cameraRotationInField.at<double>(0,0), cameraRotationInField.at<double>(0,1), cameraRotationInField.at<double>(0,2)},
			{cameraRotationInField.at<double>(1,0), cameraRotationInField.at<double>(1,1), cameraRotationInField.at<double>(1,2)},
			{cameraRotationInField.at<double>(2,0), cameraRotationInField.at<double>(2,1), cameraRotationInField.at<double>(2,2)}
		}},
		{"tagCount", tagCount},
		{"reprojErrPixels", reprojectionErrorPixels}
	};
}

void ApriltagDetector::Process(const std::vector<SourceResult>& results)
{
	for (const SourceResult& result : results)
	{
		if (result.frame.has_value())
		{
			if (m_DriverMode) {
				// still streams video (matches PhotonVision's own driver-mode behaviour) - just
				// skips the actual detection call and NT4 publish, the expensive part. Keeps the
				// same {"tags":[...],"multiTag":...} envelope as the real detection path below
				// (both empty/null) so NetworkTablesSink clears tags/* AND multitag/* to "0 tags"
				// rather than leaving stale data from before driver mode was enabled.
				// AsBgrFrame(), not AsBgr() - a bare cv::Mat republished through SourceResult's
				// implicit conversion would wrap it in a brand new, pool-ownership-less Frame,
				// which is a real use-after-recycle risk the moment `result` (this function's own
				// parameter, holding the ONLY other reference to that buffer's pool owner) goes
				// out of scope at the end of this Process() call.
				SetLatestResult(SourceResult(nlohmann::json{{"tags", nlohmann::json::array()}, {"multiTag", nullptr}}, result.frame->AsBgrFrame(), result.captureTimeUs));
				continue;
			}

			// AsGray() is free for a GRAY8/NV12-tagged Frame (no conversion needed) instead of
			// always paying for a cvtColor here - the whole point of Frame carrying a format
			// tag (ROADMAP.md Phase 3).
			const cv::Mat& gray = result.frame->AsGray();

			m_Logger->EnterLog("detecting apriltags using backend=" + m_Backend->Name());
			zarray_t* detections = m_Backend->Detect(gray);

			// Acquire()+copyTo() instead of .clone() - .clone() always mallocs a fresh buffer;
			// this recycles one from the pool when one of the right size is free. `colourOwner`
			// must be carried into the SetLatestResult call below via Frame's pool-owner
			// constructor, not dropped by passing the bare cv::Mat through the implicit
			// conversion - same hazard as AsBgrFrame's own comment describes.
			std::shared_ptr<void> colourOwner;
			const cv::Mat& sourceBgr = result.frame->AsBgr();
			cv::Mat colouredFrame = FramePool::Instance().Acquire(sourceBgr.rows, sourceBgr.cols, sourceBgr.type(), colourOwner);
			sourceBgr.copyTo(colouredFrame);

			std::vector<nlohmann::json> jsonVector;

			// ROADMAP.md Phase 7 (multi-tag PnP): accumulated across every tag this frame that
			// has both a real detection AND a known field pose, then solved once, jointly, after
			// the per-tag loop below - see the loop body for why this is more robust than any
			// one tag's own single-tag pose.
			std::vector<cv::Point3d> multiTagObjectPoints;
			std::vector<cv::Point2d> multiTagImagePoints;
			int multiTagCount = 0;

			for (int i = 0; i < zarray_size(detections); i++) {
				apriltag_detection_t* detection;
				zarray_get(detections, i, &detection);

				// estimate_tag_pose assumes a pure pinhole projection (no distortion) when it
				// reconstructs the tag's homography from the four corners. If the camera has
				// real distortion (any real lens does), pose estimation must run on undistorted
				// corner coordinates instead, or the resulting pose is systematically wrong -
				// worse the further a tag sits from the image center. The corners used for the
				// JSON payload and the drawn overlay below stay untouched: those describe where
				// the tag actually appears in this (distorted) frame.
				nlohmann::json detectionJson = {
					{"id", detection->id},
					{"center", {detection->c[0], detection->c[1]}},
					{"corners", {
						{detection->p[0][0], detection->p[0][1]},
						{detection->p[1][0], detection->p[1][1]},
						{detection->p[2][0], detection->p[2][1]},
						{detection->p[3][0], detection->p[3][1]}
					}}
				};

				// Multi-tag PnP accumulation: this tag's 4 corners, in FIELD-frame 3D (its known
				// field pose composed with its 4 local corners) paired with the SAME corners'
				// real (distorted) image pixels - cv::solvePnP takes distCoeffs directly, so
				// these stay undistorted-uncorrected here, matching multiTagObjectPoints/
				// multiTagImagePoints being fed straight into one solvePnP call below rather than
				// through the separate undistortPoints path the single-tag estimate uses.
				//
				// The local corner order/convention below (halfSize,halfSize / -halfSize,halfSize
				// / -halfSize,-halfSize / halfSize,-halfSize matched to detection->p[0..3], local
				// +Z as the tag's outward normal) is NOT guessed - it was empirically verified
				// against apriltag.c's own homography_project corner-assignment loop (confirmed
				// by reading apriltag.c directly) and cross-checked against the real
				// estimate_tag_pose() on synthetic on-axis AND rotated/off-axis test cases,
				// recovering the exact known ground-truth pose in both.
				AprilTagFieldPose fieldPose;
				if (m_HasCalibration && m_FieldLayout.TryGetTagPose(detection->id, fieldPose)) {
					double halfSize = m_DetectionInfo.tagsize / 2.0;
					cv::Vec3d localCorners[4] = {
						{-halfSize,  halfSize, 0},
						{ halfSize,  halfSize, 0},
						{ halfSize, -halfSize, 0},
						{-halfSize, -halfSize, 0},
					};
					cv::Vec3d fieldTranslation(fieldPose.translation.x, fieldPose.translation.y, fieldPose.translation.z);
					for (int corner = 0; corner < 4; corner++) {
						cv::Vec3d fieldPoint = fieldPose.rotation * localCorners[corner] + fieldTranslation;
						multiTagObjectPoints.emplace_back(fieldPoint[0], fieldPoint[1], fieldPoint[2]);
						multiTagImagePoints.emplace_back(detection->p[corner][0], detection->p[corner][1]);
					}
					multiTagCount++;
				}

				// estimate_tag_pose has no way to report "these intrinsics are degenerate" - given
				// fx=fy=0 (no calibration attached yet) it still returns, but pose.R/pose.t come
				// back as garbage/invalid pointers; dereferencing or matd_destroy-ing them corrupts
				// the heap (confirmed by reproducing standalone under gdb). Must not even attempt
				// pose estimation without valid intrinsics.
				if (m_HasCalibration) {
					apriltag_detection_t poseDetection = *detection;
					if (m_HasDistortion) {
						std::vector<cv::Point2d> distortedCorners = {
							{ detection->p[0][0], detection->p[0][1] },
							{ detection->p[1][0], detection->p[1][1] },
							{ detection->p[2][0], detection->p[2][1] },
							{ detection->p[3][0], detection->p[3][1] }
						};
						std::vector<cv::Point2d> undistortedCorners;
						// passing m_CameraMatrix as both the "new" camera matrix (P) and the
						// original one keeps the output in the same pixel scale as the input,
						// just with distortion removed - exactly what estimate_tag_pose expects
						cv::undistortPoints(distortedCorners, undistortedCorners, m_CameraMatrix, m_DistCoeffs, cv::noArray(), m_CameraMatrix);
						for (int corner = 0; corner < 4; corner++) {
							poseDetection.p[corner][0] = undistortedCorners[corner].x;
							poseDetection.p[corner][1] = undistortedCorners[corner].y;
						}
					}

					m_DetectionInfo.det = &poseDetection;
					apriltag_pose_t pose;
					double err = estimate_tag_pose(&m_DetectionInfo, &pose);

					// R is row-major 3x3 (pose.R->data[i*3+j], confirmed against
					// apriltag_pose.h/matd_t's own layout) - published whole, not decomposed into
					// Euler angles here: WPILib's Rotation3d has its own constructor taking a
					// rotation matrix directly (edu.wpi.first.math.geometry.Rotation3d(Matrix<N3,
					// N3>)), so the robot-side vendordep (Phase 7) can build a Transform3d from
					// this with no lossy intermediate representation or convention mismatch to
					// get wrong on this end. No rotation published meant no Transform3d, no pose
					// ambiguity handling, no pose estimator - the actual reason a team would
					// switch to this vendordep at all.
					detectionJson["pose"] = {
						{"x", pose.t->data[0]},
						{"y", pose.t->data[1]},
						{"z", pose.t->data[2]},
						{"R", {
							{pose.R->data[0], pose.R->data[1], pose.R->data[2]},
							{pose.R->data[3], pose.R->data[4], pose.R->data[5]},
							{pose.R->data[6], pose.R->data[7], pose.R->data[8]}
						}}
					};

					// estimate_tag_pose allocates pose.R/pose.t and documents that freeing them is
					// the caller's responsibility (see apriltag/common/matd.h) - this was never done
					matd_destroy(pose.R);
					matd_destroy(pose.t);
				}

				jsonVector.push_back(detectionJson);

				cv::line(colouredFrame, cv::Point(detection->p[0][0], detection->p[0][1]),
					cv::Point(detection->p[1][0], detection->p[1][1]),
					cv::Scalar(0, 0xff, 0), 2);
				cv::line(colouredFrame, cv::Point(detection->p[0][0], detection->p[0][1]),
					cv::Point(detection->p[3][0], detection->p[3][1]),
					cv::Scalar(0, 0, 0xff), 2);
				cv::line(colouredFrame, cv::Point(detection->p[1][0], detection->p[1][1]),
					cv::Point(detection->p[2][0], detection->p[2][1]),
					cv::Scalar(0xff, 0, 0), 2);
				cv::line(colouredFrame, cv::Point(detection->p[2][0], detection->p[2][1]),
					cv::Point(detection->p[3][0], detection->p[3][1]),
					cv::Scalar(0xff, 0, 0), 2);

				std::stringstream ss;
				ss << detection->id;
				std::string text = ss.str();
				int fontface = cv::FONT_HERSHEY_SCRIPT_SIMPLEX;
				double fontscale = 1.0;
				int baseline;
				cv::Size textsize = cv::getTextSize(text, fontface, fontscale, 2,
					&baseline);
				cv::putText(colouredFrame, text, cv::Point(detection->c[0] - textsize.width / 2,
					detection->c[1] + textsize.height / 2),
					fontface, fontscale, cv::Scalar(0xff, 0x99, 0), 2);
			}

			m_Backend->ReleaseResult(detections);

			nlohmann::json multiTagJson = SolveMultiTagPnP(
				multiTagObjectPoints, multiTagImagePoints, m_CameraMatrix, m_DistCoeffs, multiTagCount);

			SetLatestResult(SourceResult(nlohmann::json{{"tags", jsonVector}, {"multiTag", multiTagJson}},
				Frame(colouredFrame, FrameFormat::BGR24, colourOwner), result.captureTimeUs));
		}
	}
}
