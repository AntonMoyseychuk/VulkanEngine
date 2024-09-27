#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <optional>

std::optional<nlohmann::json> ParseJson(const std::filesystem::path& pathToJson) noexcept;