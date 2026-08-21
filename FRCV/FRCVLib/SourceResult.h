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

	// which bound source this result came from, filled in by ISink::ProcessingThreadLoop
	// (not by the producing ISource itself, which has no reason to know its own ID at
	// construction time inside Process()). Needed by any sink bound to more than one source
	// that must know which source a given result belongs to - e.g. NetworkTablesSink publishing
	// each bound detector into its own subtable.
	std::string sourceId;
};

