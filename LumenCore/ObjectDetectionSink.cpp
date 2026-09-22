#include "ObjectDetectionSink.h"
#include <nlohmann/json.hpp>

ObjectDetectionSink::ObjectDetectionSink(std::shared_ptr<Logger> logger, std::string id, std::shared_ptr<IDetectionBackend> backend)
	: ISource(logger, id), ISink(logger, 1, false, true, id), m_Backend(backend), m_Logger(logger)
{
	if (m_Logger) m_Logger->EnterLog("ObjectDetectionSink constructed with backend=" + (backend ? backend->Name() : "none"));
	m_DoNotLoadCaptureThread = true;
}

void ObjectDetectionSink::Process(std::vector<SourceResult> results)
{
	for (const SourceResult& result : results) {
		if (!result.frame.has_value()) continue;

		if (result.frame->empty() || !m_Backend) continue;
		const cv::Mat& sourceFrame = result.frame->AsBgr();

		if (m_DriverMode) {
			// still streams video (matches PhotonVision's own driver-mode behaviour) - skips
			// the actual inference call, the expensive part.
			SetLatestResult(SourceResult(nlohmann::json(std::vector<nlohmann::json>{}), sourceFrame, result.captureTimeUs));
			continue;
		}

		std::vector<ObjectDetection> detections = m_Backend->Infer(sourceFrame);

		cv::Mat annotatedFrame = sourceFrame.clone();
		std::vector<nlohmann::json> jsonVector;

		for (const ObjectDetection& detection : detections) {
			cv::Rect2d box = detection.GetBoundingBox();

			jsonVector.push_back(nlohmann::json{
				{"classId", detection.GetClassId()},
				{"className", detection.GetClassName()},
				{"confidence", detection.GetConfidence()},
				{"box", {box.x, box.y, box.width, box.height}}
			});

			cv::rectangle(annotatedFrame, box, cv::Scalar(0, 0xff, 0), 2);
			std::string label = detection.GetClassName() + " " + std::to_string(static_cast<int>(detection.GetConfidence() * 100)) + "%";
			cv::putText(annotatedFrame, label, cv::Point(static_cast<int>(box.x), static_cast<int>(box.y) - 5),
				cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0xff, 0), 1);
		}

		SetLatestResult(SourceResult(nlohmann::json(jsonVector), annotatedFrame, result.captureTimeUs));
	}
}
