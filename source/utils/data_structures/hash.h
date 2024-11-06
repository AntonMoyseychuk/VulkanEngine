#pragma once

#include <cstdint>
#include <type_traits>


template <typename T>
inline uint64_t amHash(const T& value) noexcept
{
    static_assert(!std::is_pointer_v<T>, "amHash can't be used for pointers. Use amHashMem");

    static std::hash<T> hasher;
    return hasher(value);
}


inline uint64_t amHashMem(const void* data, size_t size) noexcept
{
    if (!data || size == 0) {
        return 0;
    }

    const uint64_t* ptr = reinterpret_cast<const uint64_t*>(data);

    uint64_t hash = 0;
    int64_t remaining = size / sizeof(uint64_t);
    
    while (remaining > 0) {
        hash ^= *ptr++;
        hash *= 1103515245;
        hash ^= *ptr++;
        hash *= 1103515245;
        remaining -= (int64_t)sizeof(uint64_t);
    }

    const uint8_t* tail = reinterpret_cast<const uint8_t*>(ptr);
    const size_t tailLength = size % sizeof(uint64_t);

    for (size_t i = 0; i < tailLength; ++i) {
        hash ^= static_cast<uint64_t>(tail[i]) << (i * 8);
        hash *= 1103515245;
    }

    return hash;
}


namespace ds
{
    class HashBuilder
    {
    public:
        HashBuilder() = default;

        template <typename T>
        void AddValue(const T& value) noexcept
        {
            m_value *= 31ull;
            m_value += amHash(value);
        }

        void AddMemory(const void* ptr, size_t size) noexcept
        {
            m_value *= 31ull;
            m_value += amHashMem(ptr, size);
        }

        void Clear() noexcept { m_value = 0; }
        uint64_t Value() const noexcept { return m_value; }

    private:
        uint64_t m_value = 0;
    };
}