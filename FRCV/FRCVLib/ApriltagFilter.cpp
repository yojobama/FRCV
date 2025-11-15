#include "ApriltagFilter.h"
#include "FilterAnalysis.h"
#include "ISource.h"
#include "Frame.h"
#include "PreProcessor.h"
#include "FramePool.h"
#include "ApriltagDetection.h"
#include "DrawCommands.h"

#include <regex>
#include <sstream>

ApriltagFilter::ApriltagFilter(std::shared_ptr<Logger> logger, std::shared_ptr<FramePool> framePool, std::shared_ptr<PreProcessor> preProcessor) : FilterBase(logger, 1)
{
	m_Detector = apriltag_detector_create();
	apriltag_family_t* tf = tag36h11_create();
	apriltag_detector_add_family_bits(m_Detector, tf, 1);
	m_PreProcessor = preProcessor;
	m_FramePool = framePool;
}

ApriltagFilter::~ApriltagFilter()
{
	delete m_Detector;
}

FilterAnalysis ApriltagFilter::Process()
{
	// Placeholder implementation
	// Actual implementation would process frames from sources and detect AprilTags
	
	std::shared_ptr<Frame> frame = m_Sources[0]->GetLatestFrame();
	FilterAnalysis analysis(frame, 0);

	std::string returnString = "{\"detections\":[";

	if (frame != nullptr) {
		FrameSpec spec = frame->GetSpec();

		spec.SetType(CV_8UC1);
		//if (logger) logger->enterLog("spec.type after setType: " + std::to_string(spec.getType()));

		std::shared_ptr<Frame> gray = m_PreProcessor->transformFrame(frame, spec);

		std::vector<ApriltagDetection> returnVector;

		//m_Logger->EnterLog("making an image_u8_t from the opencv frame");

		image_u8_t img = {
			gray->cols,
			gray->rows,
			gray->cols,
			gray->data
		};

		zarray_t* detections = apriltag_detector_detect(m_Detector, &img);

		for (int i = 0; i < zarray_size(detections); i++) {
			apriltag_detection_t* detection;
			zarray_get(detections, i, &detection);

			apriltag_pose_t pose;
			/*double err = estimate_tag_pose(&info, &pose);*/

			returnVector.push_back(ApriltagDetection(*detection, pose));
		}

		// save the results for preview creation if preview is enabled

		for (size_t i = 0; i < returnVector.size(); ++i) {
			returnString += returnVector[i].ToString();
			if (i != returnVector.size() - 1) {
				returnString += ",";
			}
		}
		m_FramePool->ReturnFrame(gray);
	}
	else {
		//m_Logger->EnterLog(LogLevel::Error, "ApriltagSink::getResults: Frame is null");
		returnString += "{}";
	}

	returnString += "]}";
	
	analysis.SetJsonData(returnString);

	return analysis;
}

std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> ApriltagFilter::CreateDrawCommands(std::shared_ptr<FilterAnalysis> analysis)
{
	std::vector<std::shared_ptr<DrawCommands::DrawCommandBase>> commands;
	if (!analysis) return commands;

	std::string json = analysis->GetJsonData();
	if (json.empty()) return commands;

	// Find each detection block inside the JSON. The ApriltagDetection::ToString formats detection info under "detection":{...}
	std::regex detRegex("\"detection\"\\s*:\\s*\\{([\\s\\S]*?)\\}");
	auto detBegin = std::sregex_iterator(json.begin(), json.end(), detRegex);
	auto detEnd = std::sregex_iterator();

	for (auto it = detBegin; it != detEnd; ++it) {
		std::string detBlock = (*it)[1].str();

		// extract id
		int id = -1;
		std::smatch m;
		if (std::regex_search(detBlock, m, std::regex("\"id\"\\s*:\\s*(\\d+)"))) {
			id = std::stoi(m[1].str());
		}

		// extract center
		double cx = 0, cy = 0;
		if (std::regex_search(detBlock, m, std::regex("\"center\"\\s*:\\s*\\[\\s*([0-9.+\\-eE]+)\\s*,\\s*([0-9.+\\-eE]+)\\s*\\]"))) {
			cx = std::stod(m[1].str());
			cy = std::stod(m[2].str());
		}

		// extract corners: expect four pairs
		std::vector<cv::Point> corners;
		std::regex cornersRegex("\"corners\"\\s*:\\s*\\[\\s*\\[\\s*([^\\]]*?)\\s*\\],\\s*\\[\\s*([^\\]]*?)\\s*\\],\\s*\\[\\s*([^\\]]*?)\\s*\\],\\s*\\[\\s*([^\\]]*?)\\s*\\]\\s*\\]");
		if (std::regex_search(detBlock, m, cornersRegex)) {
			for (int i = 1; i <= 4; ++i) {
				std::string pair = m[i].str();
				std::regex pairRegex("\\s*([0-9.+\\-eE]+)\\s*,\\s*([0-9.+\\-eE]+)\\s*");
				std::smatch pm;
				if (std::regex_search(pair, pm, pairRegex)) {
					double x = std::stod(pm[1].str());
					double y = std::stod(pm[2].str());
					corners.emplace_back(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)));
				}
			}
		}

		// Build draw commands for this detection
		// colours: Green for outline, Red for corners, Blue for center, White for text
		cv::Scalar outlineCol(0, 255, 0);
		cv::Scalar cornerCol(0, 0, 255);
		cv::Scalar centerCol(255, 0, 0);
		cv::Scalar textCol(255, 255, 255);

		// Draw outline lines
		if (corners.size() == 4) {
			for (int i = 0; i < 4; ++i) {
				auto a = corners[i];
				auto b = corners[(i + 1) % 4];
				commands.push_back(std::make_shared<DrawCommands::DrawLineCommand>(a, b, outlineCol, 2));
			}

			// corner circles
			for (int i = 0; i < 4; ++i) {
				commands.push_back(std::make_shared<DrawCommands::DrawCircleCommand>(corners[i], 4, cornerCol, -1));
			}
		}

		// center
		commands.push_back(std::make_shared<DrawCommands::DrawCircleCommand>(cv::Point(static_cast<int>(std::lround(cx)), static_cast<int>(std::lround(cy))), 3, centerCol, 2));

		// id text
		if (id >= 0) {
			std::ostringstream ss; ss << "id:" << id;
			commands.push_back(std::make_shared<DrawCommands::PutTextCommand>(ss.str(), cv::Point(static_cast<int>(std::lround(cx)) + 10, static_cast<int>(std::lround(cy))), 0.7, textCol, 2));
		}
	}

	return commands;
}
