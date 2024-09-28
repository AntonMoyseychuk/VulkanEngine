namespace amjson
{
    template <typename T>
    inline std::vector<T> ParseArrayJson(const nlohmann::json& json) noexcept
    {
        if (json.empty()) {
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