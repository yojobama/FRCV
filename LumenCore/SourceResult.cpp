#include "SourceResult.h"
#include <chrono>

SourceResult::SourceResult()
{
	frame = std::optional<cv::Mat>();
	json = std::optional<nlohmann::json>();
}

SourceResult::SourceResult(std::optional<nlohmann::json> json, std::optional<cv::Mat> frame)
	: json(json), frame(frame)
{
}

SourceResult::SourceResult(std::optional<nlohmann::json> json, std::optional<cv::Mat> frame, uint64_t captureTimeUs)
	: json(json), frame(frame), captureTimeUs(captureTimeUs)
{
}

uint64_t SourceResult::NowUs()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}
