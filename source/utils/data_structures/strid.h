#pragma once

#include <unordered_map>
#include <string_view>
#include <string>

#include <cstdint>

#include "hash.h"


namespace ds
{
    template <typename ElemT>
    class StrIDDataStorage
    {
        template <typename T>
        friend class StrIDImpl;

    private:
        using ElementType = ElemT;
        using StringViewType = std::basic_string_view<ElementType>;
        using StringType = std::basic_string<ElementType, std::char_traits<ElementType>, std::allocator<ElementType>>;

    public:
        StrIDDataStorage();

        uint64_t Store(const StringViewType& str) noexcept;
        uint64_t Store(const ElementType* str)    noexcept { return Store(StringViewType(str)); }
        uint64_t Store(const StringType& str)     noexcept { return Store(StringViewType(str)); }

        const ElementType* Load(uint64_t id) const noexcept;

        bool IsExist(uint64_t id) const noexcept { return m_strBufLocations.find(id) != m_strBufLocations.cend(); }

    private:
        static inline constexpr uint64_t INVALID_ID_HASH = std::numeric_limits<uint64_t>::max();
        static inline constexpr size_t PREALLOCATED_IDS_COUNT = 8192ull;
        
        static inline constexpr size_t AVERAGE_STR_SIZE = 32ull;
        static inline constexpr size_t PREALLOCATED_STORAGE_SIZE = PREALLOCATED_IDS_COUNT * AVERAGE_STR_SIZE;

    private:
        // We generate a unique hash in the Store method, and we don't want to hash the already unique value again, 
        // so we use a proxy hasher
        struct StringIdHasher
        {
            inline constexpr uint64_t operator()(uint64_t id) const noexcept { return id; }
        };

        struct StringBufLocation
        {
            uint32_t offset;
            uint32_t length;
        };

        std::unordered_map<uint64_t, StringBufLocation, StringIdHasher> m_strBufLocations;
        std::vector<ElementType> m_storage;
        uint64_t m_lastAllocatedID = INVALID_ID_HASH;
    };


    template <typename ElemT>
    class StrIDImpl
    {
    public:
        using ElementType = ElemT;
        using StringViewType = std::basic_string_view<ElementType>;
        using StringType = std::basic_string<ElementType, std::char_traits<ElementType>, std::allocator<ElementType>>;

    private:
        using StrIDDataStorageType = StrIDDataStorage<ElementType>;

    public:
        StrIDImpl() = default;
        StrIDImpl(const ElementType* str);
        StrIDImpl(const StringType& str);
        StrIDImpl(const StringViewType& str);

        StrIDImpl& operator=(const ElementType* str)    noexcept;
        StrIDImpl& operator=(const StringType& str)     noexcept { return operator=(str.c_str()); }
        StrIDImpl& operator=(const StringViewType& str) noexcept { return operator=(str.data()); }

        const ElementType* CStr() const noexcept;

        bool operator==(StrIDImpl strId) const noexcept { return m_id == strId.m_id; }
        bool operator!=(StrIDImpl strId) const noexcept { return m_id != strId.m_id; }
        bool operator<(StrIDImpl strId) const noexcept { return m_id < strId.m_id; }
        bool operator>(StrIDImpl strId) const noexcept { return m_id > strId.m_id; }
        bool operator<=(StrIDImpl strId) const noexcept { return m_id <= strId.m_id; }
        bool operator>=(StrIDImpl strId) const noexcept { return m_id >= strId.m_id; }

        uint64_t GetId() const noexcept { return m_id; }
        uint64_t Hash() const noexcept { return m_id; }

        bool IsValid() const noexcept { return m_id != StrIDDataStorageType::INVALID_ID_HASH; }

    private:
        static inline StrIDDataStorageType s_storage;

    private:
        uint64_t m_id = StrIDDataStorageType::INVALID_ID_HASH;
    };


    using StrID = StrIDImpl<char>;
    using WStrID = StrIDImpl<wchar_t>;
}

namespace std {
    template<>
    struct hash<ds::StrID> {
        uint64_t operator()(const ds::StrID& id) const { return id.Hash(); }
    };

    template<>
    struct hash<ds::WStrID> {
        uint64_t operator()(const ds::WStrID& id) const { return id.Hash(); }
    };
}


#include "strid.hpp"