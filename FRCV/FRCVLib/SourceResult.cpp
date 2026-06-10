#include "SourceResult.h"

SourceResult::SourceResult()
{
	frame = std::optional<cv::Mat>();
	json = std::optional<nlohmann::json>();
}

SourceResult::SourceResult(std::optional<nlohmann::json> json, std::optional<cv::Mat> frame)
	: json(json), frame(frame)
{
}