#pragma once

#include <nlohmann/json.hpp>
#include "Frame.h"
#include <optional>
#include <memory>

class SourceResult
{
public:
	SourceResult();
	SourceResult(std::optional<nlohmann::json> json, std::optional<std::shared_ptr<Frame>> frame);
	std::optional<nlohmann::json> json;
	std::optional<std::shared_ptr<Frame>> frame;
};

