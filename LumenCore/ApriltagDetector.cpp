#include "ApriltagDetector.h"
#include "ApriltagDetection.h"
#include "CameraCalibrationResult.h"
#include "CpuApriltagBackend.h"
#ifdef LUMEN_WITH_VULKAN_APRILTAG
#include "VkApriltagBackend.h"
#endif

ApriltagDetector::ApriltagDetector(std::shared_ptr<Logger> logger, std::string id, CameraCalibrationResult cameraCalibrationResult,
	double tagSize, ApriltagBackendKind backendKind, int frameWidth, int frameHeight)
	: ISource(logger, id), ISink(logger, 1, false, true, id)
{
	if (logger) logger->EnterLog("ApriltagDetector constructed");

	if (backendKind == APRILTAG_BACKEND_VULKAN) {
#ifdef LUMEN_WITH_VULKAN_APRILTAG
		try {
			m_Backend = std::make_unique<VkApriltagBackend>(frameWidth, frameHeight);
		} catch (const std::exception& e) {
			// no usable Vulkan compute device, or GpuDetector/pipeline setup failed - fall back
			// to CPU rather than fail to construct at all (plan phase 5, item 5)
			if (logger) logger->EnterLog(LogLevel::Warning,
				std::string("Vulkan AprilTag backend unavailable (") + e.what() + "), falling back to CPU");
			m_Backend = std::make_unique<CpuApriltagBackend>();
		}
#else
		if (logger) logger->EnterLog(LogLevel::Warning,
			"Vulkan AprilTag backend requested but LUMEN_WITH_VULKAN_APRILTAG was not compiled in, falling back to CPU");
		m_Backend = std::make_unique<CpuApriltagBackend>();
#endif
	} else {
		m_Backend = std::make_unique<CpuApriltagBackend>();
	}

	m_DetectionInfo.tagsize = tagSize;
	m_DetectionInfo.fx = cameraCalibrationResult.fx;
	m_DetectionInfo.fy = cameraCalibrationResult.fy;
	m_DetectionInfo.cx = cameraCalibrationResult.cx;
	m_DetectionInfo.cy = cameraCalibrationResult.cy;
	m_HasCalibration = cameraCalibrationResult.fx > 0.0 && cameraCalibrationResult.fy > 0.0;

	if (cameraCalibrationResult.HasDistortion()) {
		m_HasDistortion = true;
		m_CameraMatrix = (cv::Mat_<double>(3, 3) <<
			cameraCalibrationResult.fx, 0, cameraCalibrationResult.cx,
			0, cameraCalibrationResult.fy, cameraCalibrationResult.cy,
			0, 0, 1);
		m_DistCoeffs = cv::Mat(cameraCalibrationResult.distCoeffs, true /* copy */);
	}

	m_Logger = logger;

	m_DoNotLoadCaptureThread = true;
}

ApriltagDetector::~ApriltagDetector() = default;

std::string ApriltagDetector::GetBackendName() const
{
	return m_Backend->Name();
}

void ApriltagDetector::Process(std::vector<SourceResult> results)
{
	for (const SourceResult& result : results)
	{
		if (result.frame.has_value())
		{
			if (m_DriverMode) {
				// still streams video (matches PhotonVision's own driver-mode behaviour) - just
				// skips the actual detection call and NT4 publish, the expensive part.
				SetLatestResult(SourceResult(nlohmann::json(std::vector<nlohmann::json>{}), result.frame->AsBgr()));
				continue;
			}

			// AsGray() is free for a GRAY8/NV12-tagged Frame (no conversion needed) instead of
			// always paying for a cvtColor here - the whole point of Frame carrying a format
			// tag (ROADMAP.md Phase 3).
			const cv::Mat& gray = result.frame->AsGray();

			m_Logger->EnterLog("detecting apriltags using backend=" + m_Backend->Name());
			zarray_t* detections = m_Backend->Detect(gray);

			cv::Mat colouredFrame = result.frame->AsBgr().clone();

			std::vector<nlohmann::json> jsonVector;

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

			SetLatestResult(SourceResult(nlohmann::json(jsonVector), colouredFrame));
		}
	}
}
