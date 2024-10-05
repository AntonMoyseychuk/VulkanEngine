namespace amjson
{
    template <typename T>
    inline std::vector<T> ParseJsonArray(const nlohmann::json& json) noexcept
    {
        if (json.empty()) {
            AM_LOG_WARN("Json node is empty");
            return {};
        }

        if (!json.is_array()) {
            AM_LOG_WARN("Trying to parse Json node to invalid type (array)");
            return {};
        }

        std::vector<T> result;
        result.reserve(json.size());

        for (const auto& item : json.items()) {
            result.emplace_back(item.value().get<T>());
        }

        return result;
    }
}