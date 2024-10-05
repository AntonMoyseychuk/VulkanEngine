#include "../../pch.h"

#include "json.h"

namespace amjson
{
    std::optional<nlohmann::json> ParseJson(const std::filesystem::path& pathToJson) noexcept
    {
        if (!std::filesystem::exists(pathToJson)) {
            AM_LOG_WARN("Json parsing error. File {} doesn't exist.", pathToJson.string());
            return {};
        }

        std::ifstream jsonFile(pathToJson);
        if (!jsonFile.is_open()) {
            AM_LOG_WARN("Json parsing error. Failed to open {} file.", pathToJson.string());
            return {};
        }

        try {
            return nlohmann::json::parse(jsonFile);
        } catch(const nlohmann::json::parse_error& e) {
            AM_LOG_WARN("Json parsing error. Error: {}", e.what());
            return {};
        }
    }
}
