#pragma once

#include <unordered_map>

#include "utils/file/file.h"
#include "shaderid.h"


struct VulkanShaderCompiledCodeBuffer
{
    bool IsValid() const noexcept { return pCode && sizeInU32 > 0 && hash != ShaderID::INVALID_HASH; }

    const uint32_t* pCode = nullptr;
    size_t sizeInU32      = 0;
    
    uint64_t hash         = ShaderID::INVALID_HASH;
};


class VulkanShaderCache
{
private:
    struct VulkanShaderCacheEntryLocation
    {
        size_t beginPosition;
        size_t sizeInU8;
    };

public:
    bool Load(const fs::path& shaderCacheFilepath) noexcept;
    void Clear() noexcept;

    bool IsEmpty() const noexcept { return m_cacheLocations.empty(); }
    size_t GetCacheEntryCount() const noexcept { return m_cacheLocations.size(); }

    bool Contains(uint64_t shaderHash) const noexcept;
    bool Contains(const ShaderID& id) const noexcept;

    VulkanShaderCompiledCodeBuffer GetShaderPrecompiledCode(uint64_t shaderHash) const noexcept;
    VulkanShaderCompiledCodeBuffer GetShaderPrecompiledCode(const ShaderID& id) const noexcept;
    
    void AddCacheEntryToSubmitBuffer(const ShaderID& id, const std::vector<uint8_t>& shaderCompiledCode) noexcept;

    void Submit(const fs::path& shaderCacheFilepath) noexcept;

    template <typename Func>
    void ForEachShaderCacheEntry(Func func) const noexcept;

private:
    VulkanShaderCompiledCodeBuffer GetShaderPrecompiledCode(const VulkanShaderCacheEntryLocation& location) const noexcept;

private:
    std::unordered_map<ShaderIDProxy, VulkanShaderCacheEntryLocation> m_cacheLocations;
    std::vector<uint8_t> m_cacheStorage;
    
    std::vector<uint8_t> m_submitBuffer;
    uint32_t m_submitionCacheEntryCount = 0;
};


template <typename Func>
inline void VulkanShaderCache::ForEachShaderCacheEntry(Func func) const noexcept
{
    for (const VulkanShaderCacheEntryLocation& location : m_cacheLocations) {
        VulkanShaderCompiledCodeBuffer buffer = GetShaderPrecompiledCode(location);
        func(buffer);
    }
}
