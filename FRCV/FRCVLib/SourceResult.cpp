#include "SourceResult.h"

SourceResult::SourceResult()
{
	frame = std::optional<std::shared_ptr<Frame>>();
	json = std::optional<nlohmann::json>();
}

SourceResult::SourceResult(std::optional<nlohmann::json> json, std::optional<std::shared_ptr<Frame>> frame)
	: json(json), frame(frame)
{
}