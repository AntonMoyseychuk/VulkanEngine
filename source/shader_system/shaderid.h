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
    void ClearBits() noexcept;

    bool IsDefineBit(size_t index) const noexcept;

    bool IsHashValid() const noexcept { return Hash() != INVALID_HASH; }

    uint64_t Hash() const noexcept;

    bool operator==(const ShaderID& id) const noexcept { return m_filepath == id.m_filepath && m_defineBits == id.m_defineBits; }
    bool operator!=(const ShaderID& id) const noexcept { return !operator==(id); }
    bool operator<(const ShaderID& id) const noexcept { return m_filepath < id.m_filepath; }
    bool operator>(const ShaderID& id) const noexcept { return m_filepath > id.m_filepath; }
    bool operator<=(const ShaderID& id) const noexcept { return operator<(id) || operator==(id); }
    bool operator>=(const ShaderID& id) const noexcept { return operator>(id) || operator==(id); }

    void SetFilepath(ds::StrID filepath) noexcept { m_filepath = filepath; }
    ds::StrID GetFilepath() const noexcept { return m_filepath; }

private:
    std::bitset<MAX_SHADER_DEFINES_COUNT> m_defineBits;
    ds::StrID m_filepath;
};


class ShaderIDProxy
{
public:
    ShaderIDProxy() = default;
    ShaderIDProxy(const ShaderID& id);
    explicit ShaderIDProxy(uint64_t shaderHash);

    bool IsValid() const noexcept { return m_hash != ShaderID::INVALID_HASH; }

    uint64_t Hash() const noexcept { return m_hash; }

    bool operator==(ShaderIDProxy other) const noexcept { return m_hash == other.m_hash; }
    bool operator!=(ShaderIDProxy other) const noexcept { return m_hash != other.m_hash; }
    bool operator<(ShaderIDProxy other) const noexcept  { return m_hash < other.m_hash; }
    bool operator>(ShaderIDProxy other) const noexcept  { return m_hash > other.m_hash; }
    bool operator<=(ShaderIDProxy other) const noexcept { return m_hash <= other.m_hash; }
    bool operator>=(ShaderIDProxy other) const noexcept { return m_hash >= other.m_hash; }

private:
    uint64_t m_hash = ShaderID::INVALID_HASH;
};


namespace std {
    template<>
    struct hash<ShaderID> {
        uint64_t operator()(const ShaderID& id) const { return id.Hash(); }
    };

    template<>
    struct hash<ShaderIDProxy> {
        uint64_t operator()(const ShaderIDProxy& idProxy) const { return idProxy.Hash(); }
    };
}