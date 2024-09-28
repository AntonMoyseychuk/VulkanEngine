#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <optional>
#include <vector>


namespace amjson
{
    std::optional<nlohmann::json> ParseJson(const std::filesystem::path& pathToJson) noexcept;

    template <typename T>
    std::vector<T> ParseArrayJson(const nlohmann::json& json) noexcept;
}

#include "json.hpp"