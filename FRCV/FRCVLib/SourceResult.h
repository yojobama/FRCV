#pragma once

#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <memory>

class SourceResult
{
public:
	SourceResult();
	SourceResult(std::optional<nlohmann::json> json, std::optional<cv::Mat> frame);
	std::optional<nlohmann::json> json;
	std::optional<cv::Mat> frame;
};

