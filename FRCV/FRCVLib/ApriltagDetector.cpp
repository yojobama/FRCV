#include "ApriltagDetector.h"
#include "ApriltagDetection.h"
#include "CameraCalibrationResult.h"

ApriltagDetector::ApriltagDetector(std::shared_ptr<Logger> logger, std::string id, CameraCalibrationResult cameraCalibrationResult, double tagSize) : ISource(logger, id), ISink(logger, 1, false, true, id)
{
	if (logger) logger->EnterLog("ApriltagDetector constructed");
	this->m_Family = tag36h11_create();
	this->m_Detector = apriltag_detector_create();
	apriltag_detector_add_family(this->m_Detector, this->m_Family);

	m_DetectionInfo.tagsize = tagSize;
	m_DetectionInfo.fx = cameraCalibrationResult.fx;
	m_DetectionInfo.fy = cameraCalibrationResult.fy;
	m_DetectionInfo.cx = cameraCalibrationResult.cx;
	m_DetectionInfo.cy = cameraCalibrationResult.cy;

	m_Logger = logger;

	m_DoNotLoadCaptureThread = true;
}

ApriltagDetector::~ApriltagDetector()
{
	delete m_Family;
	delete m_Detector;
}

void ApriltagDetector::Process(std::vector<SourceResult> results)
{
	for (const SourceResult& result : results) 
	{
		if (result.frame.has_value()) 
		{
			const cv::Mat& sourceFrame = result.frame.value();
			cv::Mat gray = cv::Mat(sourceFrame.rows, sourceFrame.cols, CV_8UC1);
			cv::cvtColor(sourceFrame, gray, cv::COLOR_BGR2GRAY);

			std::vector<ApriltagDetection> returnVector;

			m_Logger->EnterLog("making an image_u8_t from the opencv frame");

			image_u8_t img = {
				gray.cols,
				gray.rows,
				gray.cols,
				gray.data
			};

			m_Logger->EnterLog("detecting apriltags using the detector");
			zarray_t* detections = apriltag_detector_detect(m_Detector, &img);

			cv::Mat colouredFrame = sourceFrame.clone();

			std::vector<nlohmann::json> jsonVector;

			for (int i = 0; i < zarray_size(detections); i++) {
				apriltag_detection_t* detection;
				zarray_get(detections, i, &detection);

				m_DetectionInfo.det = detection;
				apriltag_pose_t pose;
				double err = estimate_tag_pose(&m_DetectionInfo, &pose);

				jsonVector.push_back(nlohmann::json{
					{"id", detection->id},
					{"center", {detection->c[0], detection->c[1]}},
					{"corners", {
						{detection->p[0][0], detection->p[0][1]},
						{detection->p[1][0], detection->p[1][1]},
						{detection->p[2][0], detection->p[2][1]},
						{detection->p[3][0], detection->p[3][1]}
					}},
					{"pose", {
						{"x", pose.t->data[0]},
						{"y", pose.t->data[1]},
						{"z", pose.t->data[2]},
						//{"yaw", pose.R->data[0]},
						//{"pitch", pose.R->data[1]},
						//{"roll", pose.R->data[2]}
					}}
					});

				returnVector.push_back(ApriltagDetection(*detection, pose));

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

			apriltag_detections_destroy(detections);

			SetLatestResult(SourceResult(nlohmann::json(jsonVector), colouredFrame));
		}
	}
}
