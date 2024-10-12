#include "pch.h"

#include "json.h"

#include "utils/debug/assertion.h"


namespace amjson
{
    std::optional<nlohmann::json> ParseJson(const fs::path& pathToJson) noexcept
    {
        if (!fs::exists(pathToJson)) {
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
