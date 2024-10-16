#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <optional>
#include <vector>
#include <string>
#include <string_view>

#include "utils/file/file.h"


namespace amjson
{
    std::optional<nlohmann::json> ParseJson(const fs::path& pathToJson) noexcept;

    nlohmann::json& GetJsonSubNode(nlohmann::json& rootNode, const char* pNodeName) noexcept;
    nlohmann::json& GetJsonSubNode(nlohmann::json& rootNode, const std::string& nodeName) noexcept;
    nlohmann::json& GetJsonSubNode(nlohmann::json& rootNode, const std::string_view& nodeName) noexcept;

    const nlohmann::json& GetJsonSubNode(const nlohmann::json& rootNode, const char* pNodeName) noexcept;
    const nlohmann::json& GetJsonSubNode(const nlohmann::json& rootNode, const std::string& nodeName) noexcept;
    const nlohmann::json& GetJsonSubNode(const nlohmann::json& rootNode, const std::string_view& nodeName) noexcept;

    template <typename T>
    std::vector<T> ParseJsonSubNodeToArray(const nlohmann::json& rootNode, const char* pNodeName) noexcept;
    template <typename T>
    std::vector<T> ParseJsonSubNodeToArray(const nlohmann::json& rootNode, const std::string& nodeName) noexcept;
    template <typename T>
    std::vector<T> ParseJsonSubNodeToArray(const nlohmann::json& rootNode, const std::string_view& nodeName) noexcept;
}

#include "json.hpp"