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
    
    
    nlohmann::json& GetJsonSubNode(nlohmann::json& rootNode, const char* pNodeName) noexcept
    {
        AM_ASSERT(pNodeName, "pNodeName is nullptr");
        AM_ASSERT(rootNode.contains(pNodeName), "Root node doesn't contain '{}' subnode", pNodeName);

        return rootNode[pNodeName];
    }
    
    
    nlohmann::json& GetJsonSubNode(nlohmann::json& rootNode, const std::string& nodeName) noexcept
    {
        return GetJsonSubNode(rootNode, nodeName.c_str());
    }
    
    
    nlohmann::json& GetJsonSubNode(nlohmann::json& rootNode, const std::string_view& nodeName) noexcept
    {
        return GetJsonSubNode(rootNode, nodeName.data());
    }


    const nlohmann::json& GetJsonSubNode(const nlohmann::json& rootNode, const char* pNodeName) noexcept
    {
        AM_ASSERT(pNodeName, "pNodeName is nullptr");
        AM_ASSERT(rootNode.contains(pNodeName), "Root node doesn't contain '{}' subnode", pNodeName);

        return rootNode[pNodeName];
    }
    
    
    const nlohmann::json& GetJsonSubNode(const nlohmann::json& rootNode, const std::string& nodeName) noexcept
    {
        return GetJsonSubNode(rootNode, nodeName.c_str());
    }
    
    
    const nlohmann::json& GetJsonSubNode(const nlohmann::json& rootNode, const std::string_view& nodeName) noexcept
    {
        return GetJsonSubNode(rootNode, nodeName.data());
    }
}
