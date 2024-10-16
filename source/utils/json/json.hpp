namespace amjson
{
    template <typename T>
    inline std::vector<T> ParseJsonSubNodeToArray(const nlohmann::json& rootNode, const char* pNodeName) noexcept
    {
        AM_ASSERT(pNodeName, "pNodeName is nullptr");

        const nlohmann::json& jsonSubNode = GetJsonSubNode(rootNode, pNodeName);

        if (jsonSubNode.empty()) {
            AM_LOG_WARN("Json node '{}' is empty", pNodeName);
            return {};
        }

        if (!jsonSubNode.is_array()) {
            AM_LOG_WARN("Trying to parse Json node '{}' to invalid type ({})", pNodeName, jsonSubNode.type_name());
            return {};
        }

        std::vector<T> result;
        result.reserve(jsonSubNode.size());

        for (const auto& item : jsonSubNode.items()) {
            result.emplace_back(item.value().get<T>());
        }

        return result;
    }


    template <typename T>
    inline std::vector<T> ParseJsonSubNodeToArray(const nlohmann::json& rootNode, const std::string& nodeName) noexcept
    {
        return ParseJsonSubNodeToArray<T>(rootNode, nodeName.c_str());
    }


    template <typename T>
    inline std::vector<T> ParseJsonSubNodeToArray(const nlohmann::json& rootNode, const std::string_view& nodeName) noexcept
    {
        return ParseJsonSubNodeToArray<T>(rootNode, nodeName.data());
    }
}