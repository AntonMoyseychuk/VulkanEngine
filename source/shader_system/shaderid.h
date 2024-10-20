#pragma once

#include <cstdint>
#include <bitset>

#include "utils/data_structures/strid.h"


class ShaderID
{
public:
    static inline constexpr size_t MAX_SHADER_DEFINES_COUNT = 256;
    static inline constexpr uint64_t INVALID_HASH = std::numeric_limits<uint64_t>::max();

public:
    ShaderID() = default;
    ShaderID(ds::StrID filepath);
    ShaderID(ds::StrID filepath, const std::bitset<MAX_SHADER_DEFINES_COUNT>& defineBits);

    void SetDefineBit(size_t index, bool value = true) noexcept;
    void ClearDefineBit(size_t index) noexcept;

    bool IsDefineBit(size_t index) const noexcept;

    bool IsHashValid() const noexcept { return Hash() != INVALID_HASH; }

    uint64_t Hash() const noexcept;

    bool operator==(const ShaderID& id) const noexcept { return m_filepath == id.m_filepath && m_defineBits == id.m_defineBits; }
    bool operator!=(const ShaderID& id) const noexcept { return !operator==(id); }
    bool operator<(const ShaderID& id) const noexcept { return m_filepath < id.m_filepath; }
    bool operator>(const ShaderID& id) const noexcept { return m_filepath > id.m_filepath; }
    bool operator<=(const ShaderID& id) const noexcept { return operator<(id) || operator==(id); }
    bool operator>=(const ShaderID& id) const noexcept { return operator>(id) || operator==(id); }

    ds::StrID GetFilepath() const noexcept { return m_filepath; }

private:
    ds::StrID m_filepath;
    std::bitset<MAX_SHADER_DEFINES_COUNT> m_defineBits;
};


namespace std {
    template<>
    struct hash<ShaderID> {
        uint64_t operator()(const ShaderID& id) const { return id.Hash(); }
    };
}