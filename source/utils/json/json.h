#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <optional>
#include <vector>

#include "utils/file/file.h"


namespace amjson
{
    std::optional<nlohmann::json> ParseJson(const fs::path& pathToJson) noexcept;

    template <typename T>
    std::vector<T> ParseJsonArray(const nlohmann::json& json) noexcept;
}

#include "json.hpp"