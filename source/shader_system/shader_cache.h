#pragma once

#include <unordered_map>

#include "utils/file/file.h"
#include "shaderid.h"


struct VulkanShaderCompiledCodeBuffer
{
    bool IsValid() const noexcept { return pCode && sizeInU32 > 0; }

    const uint32_t* pCode = nullptr;
    size_t sizeInU32 = 0;
};


class VulkanShaderCache
{
public:
    bool Load(const fs::path& shaderCacheDirectory) noexcept;
    void Terminate() noexcept;

    VulkanShaderCompiledCodeBuffer GetShaderPrecompiledCode(const ShaderID& id) const noexcept;

    void Store(const fs::path& shaderCacheDirectory, const ShaderID& id, const std::vector<uint8_t>& shaderCompiledCode) const noexcept;

private:
    struct VulkanShaderCachePosSize
    {
        size_t beginPosition;
        size_t sizeInU8;
    };

    struct ShaderIDProxy
    {
        bool IsValid() const noexcept { return hash != ShaderID::INVALID_HASH; }

        bool operator==(ShaderIDProxy other) const noexcept { return hash == other.hash; }
        bool operator!=(ShaderIDProxy other) const noexcept { return hash != other.hash; }
        bool operator<(ShaderIDProxy other) const noexcept  { return hash < other.hash; }
        bool operator>(ShaderIDProxy other) const noexcept  { return hash > other.hash; }
        bool operator<=(ShaderIDProxy other) const noexcept { return hash <= other.hash; }
        bool operator>=(ShaderIDProxy other) const noexcept { return hash >= other.hash; }

        uint64_t hash = ShaderID::INVALID_HASH;
    };

    struct ShaderIDProxyHasher {
        uint64_t operator()(const ShaderIDProxy& proxy) const noexcept;
    };

    std::unordered_map<ShaderIDProxy, VulkanShaderCachePosSize, ShaderIDProxyHasher> m_cacheLocations;
    std::vector<uint8_t> m_cacheStorage;

    std::vector<fs::path> m_cachedFilepathsToSPVFiles;
};