#include "../../pch.h"

#include "json.h"


std::optional<nlohmann::json> ParseJson(const std::filesystem::path& pathToJson) noexcept
{
    if (!std::filesystem::exists(pathToJson)) {
        AM_LOG_WARN("Json parsing error. File {} doesn't exist.", pathToJson.string());
        return {};
    }

    std::ifstream jsonFile(pathToJson);
    if (!jsonFile.is_open()) {
        AM_LOG_WARN("Json parsing error. Failed to open %s file.", pathToJson.string());
        return {};
    }

    return nlohmann::json::parse(jsonFile);
}