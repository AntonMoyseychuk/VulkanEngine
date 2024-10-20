#pragma once

#include <unordered_map>
#include <string_view>
#include <string>

#include <cstdint>

namespace ds
{
    class StrIDDataStorage
    {
    public:
        StrIDDataStorage();

        uint64_t Store(const std::string_view& str) noexcept;
        uint64_t Store(const char* str)             noexcept { return Store(std::string_view(str)); }
        uint64_t Store(const std::string& str)      noexcept { return Store(std::string_view(str)); }

        const std::string* Load(uint64_t id) const noexcept;

        const bool IsExist(uint64_t id) const noexcept { return m_storage.find(id) != m_storage.cend(); }

    public:
        static uint64_t GetInvalidIDHash() noexcept { return INVALID_ID_HASH; }

    private:
        static inline std::hash<std::string_view> s_hasher;

        static inline constexpr uint64_t INVALID_ID_HASH = std::numeric_limits<uint64_t>::max();
        static inline constexpr size_t PREALLOCATED_IDS_COUNT = 2048;

    private:
        std::unordered_map<uint64_t, std::string> m_storage;
    };


    class StrID
    {
    public:
        StrID() = default;
        StrID(const char* str);
        StrID(const std::string& str);
        StrID(const std::string_view& str);

        StrID& operator=(const char* str)             noexcept;
        StrID& operator=(const std::string& str)      noexcept { return operator=(str.c_str()); }
        StrID& operator=(const std::string_view& str) noexcept { return operator=(str.data()); }

        const char* CStr() const noexcept;

        const std::string* String() const noexcept { return s_storage.Load(m_id); }

        bool operator==(StrID strId) const noexcept { return m_id == strId.m_id; }
        bool operator!=(StrID strId) const noexcept { return m_id != strId.m_id; }
        bool operator<(StrID strId) const noexcept { return m_id < strId.m_id; }
        bool operator>(StrID strId) const noexcept { return m_id > strId.m_id; }
        bool operator<=(StrID strId) const noexcept { return m_id <= strId.m_id; }
        bool operator>=(StrID strId) const noexcept { return m_id >= strId.m_id; }

        uint64_t GetId() const noexcept { return m_id; }
        uint64_t Hash() const noexcept { return m_id; }

        bool IsValid() const noexcept { return m_id != StrIDDataStorage::GetInvalidIDHash(); }

    private:
        static inline StrIDDataStorage s_storage;

    private:
        uint64_t m_id = StrIDDataStorage::GetInvalidIDHash();
    };
}


namespace std {
    template<>
    struct hash<ds::StrID> {
        uint64_t operator()(const ds::StrID& id) const { return id.Hash(); }
    };
}