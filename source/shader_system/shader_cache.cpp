#include "pch.h"

#include "shader_cache.h"

#include "utils/debug/assertion.h"
#include "utils/file/file.h"


static constexpr size_t AM_SHADER_CACHE_SUBMITION_PREALLOCATION_SIZE = 4 << 20;


bool VulkanShaderCache::Load(const fs::path& shaderCacheFilepath) noexcept
{
    // Shader cache structure:
    // 4 bytes - cache entries count
    // Cache entries:
    //      4 bytes - code size
    //      8 bytes - hash
    //    ... bytes - code
    
    Clear();

    m_cacheStorage.reserve(AM_SHADER_CACHE_SUBMITION_PREALLOCATION_SIZE);

    ReadBinaryFile(shaderCacheFilepath, m_cacheStorage);

    if (m_cacheStorage.empty()) {
        return false;
    }

    const uint8_t* pStorageBeginU8  = m_cacheStorage.data();
    const uint8_t* pStorageEndU8    = m_cacheStorage.data() + m_cacheStorage.size();

    const uint32_t cacheEntryCount = *((const uint32_t*)pStorageBeginU8);
    m_cacheLocations.reserve(cacheEntryCount * 2);

    const uint8_t* pCacheEntry = pStorageBeginU8 + sizeof(uint32_t);
    while (pCacheEntry < pStorageEndU8) {
        uint32_t cacheEntrySize = 0;
        memcpy_s(&cacheEntrySize, sizeof(uint32_t), pCacheEntry, sizeof(uint32_t));

        pCacheEntry += sizeof(uint32_t);

        ShaderIDProxy id;
        memcpy_s(&id, sizeof(ShaderIDProxy), pCacheEntry, sizeof(ShaderIDProxy));

        pCacheEntry += sizeof(ShaderIDProxy);

        VulkanShaderCacheEntryLocation entryLocation;
        entryLocation.beginPosition = pCacheEntry - pStorageBeginU8;
        entryLocation.sizeInU8 = cacheEntrySize;

        m_cacheLocations[id] = entryLocation;

        pCacheEntry += cacheEntrySize;
    }

    return true;
}


void VulkanShaderCache::Clear() noexcept
{
    m_cacheLocations.clear();
    m_cacheStorage.clear();
}


bool VulkanShaderCache::Contains(uint64_t shaderHash) const noexcept
{
    return m_cacheLocations.find(ShaderIDProxy(shaderHash)) != m_cacheLocations.cend();
}


bool VulkanShaderCache::Contains(const ShaderID &id) const noexcept
{
    return m_cacheLocations.find(ShaderIDProxy(id)) != m_cacheLocations.cend();
}


VulkanShaderCompiledCodeBuffer VulkanShaderCache::GetShaderPrecompiledCode(uint64_t shaderHash) const noexcept
{
    ShaderIDProxy idProxy(shaderHash);

    const auto codeLocation = m_cacheLocations.find(idProxy);

    if (codeLocation == m_cacheLocations.cend()) {
        AM_ASSERT_GRAPHICS_API(false, "Invalid shader id");
        return {};
    }

    const VulkanShaderCacheEntryLocation& cacheLocation = codeLocation->second;

    if (cacheLocation.beginPosition + cacheLocation.sizeInU8 > m_cacheStorage.size()) {
        AM_ASSERT_GRAPHICS_API(false, "Invalid shader cache buffer position + size");
        return {};
    }

    VulkanShaderCompiledCodeBuffer codeBuffer = {};
    codeBuffer.pCode = reinterpret_cast<const uint32_t*>(&m_cacheStorage[cacheLocation.beginPosition]);
    
    if (cacheLocation.sizeInU8 % sizeof(uint32_t) != 0) {
        AM_ASSERT_GRAPHICS_API(false, "SPIR-V code size must be multiple of sizeof(uint32_t)");
        return {};
    }

    codeBuffer.sizeInU32 = cacheLocation.sizeInU8 / sizeof(uint32_t);
    codeBuffer.hash = shaderHash;

    return codeBuffer;
}


VulkanShaderCompiledCodeBuffer VulkanShaderCache::GetShaderPrecompiledCode(const ShaderID & id) const noexcept
{
    return GetShaderPrecompiledCode(id.Hash());
}


void VulkanShaderCache::AddCacheEntryToSubmitBuffer(const ShaderID &id, const std::vector<uint8_t> &shaderCompiledCode) noexcept
{
    AddCacheEntryToSubmitBuffer(id, shaderCompiledCode.data(), shaderCompiledCode.size());
}


void VulkanShaderCache::AddCacheEntryToSubmitBuffer(const ShaderID &id, const uint8_t *pShaderCompiledCode, size_t codeSize) noexcept
{
    AddCacheEntryToSubmitBuffer(ShaderIDProxy(id), pShaderCompiledCode, codeSize);
}


void VulkanShaderCache::AddCacheEntryToSubmitBuffer(ShaderIDProxy idProxy, const uint8_t *pShaderCompiledCode, size_t codeSize) noexcept
{
    AM_ASSERT_GRAPHICS_API(pShaderCompiledCode != nullptr, "pShaderCompiledCode is nullptr");
    AM_ASSERT_GRAPHICS_API(codeSize != 0, "Compiled code size is 0. Cache entry {}", idProxy.Hash());

    const size_t oldStorageSize = m_cacheStorage.empty() ? sizeof(uint32_t) : m_cacheStorage.size();
    const size_t newStorageSize = oldStorageSize + sizeof(uint32_t) + sizeof(ShaderIDProxy) + codeSize;

    m_cacheStorage.resize(newStorageSize);

    uint8_t* pStorageBeginU8  = m_cacheStorage.data();
    uint8_t* pStorageEndU8    = m_cacheStorage.data() + m_cacheStorage.size();

    uint8_t* pCacheEntrySizeBuffer = pStorageBeginU8 + oldStorageSize;
    memcpy_s(pCacheEntrySizeBuffer, sizeof(uint32_t), &codeSize, sizeof(uint32_t));

    uint8_t* pShaderHashIdBuffer = pCacheEntrySizeBuffer + sizeof(uint32_t);
    memcpy_s(pShaderHashIdBuffer, sizeof(ShaderIDProxy), &idProxy, sizeof(ShaderIDProxy));

    uint8_t* pShaderCodeBuffer = pShaderHashIdBuffer + sizeof(ShaderIDProxy);
    memcpy_s(pShaderCodeBuffer, codeSize, pShaderCompiledCode, codeSize);

    uint32_t& cacheEntryCount = *((uint32_t*)pStorageBeginU8);
    ++cacheEntryCount;
}


void VulkanShaderCache::Submit(const fs::path &shaderCacheFilepath) noexcept
{
    WriteBinaryFile(shaderCacheFilepath, m_cacheStorage.data(), m_cacheStorage.size());
}


VulkanShaderCompiledCodeBuffer VulkanShaderCache::GetShaderPrecompiledCode(const VulkanShaderCacheEntryLocation& location) const noexcept
{
    AM_ASSERT_GRAPHICS_API(location.sizeInU8 % sizeof(uint32_t) == 0, "Shader cache entry size is not multiple of sizeof(uint32_t)");

    VulkanShaderCompiledCodeBuffer buffer = {};

    buffer.sizeInU32    = location.sizeInU8 / sizeof(uint32_t);
    buffer.pCode        = (const uint32_t*)(m_cacheStorage.data() + location.beginPosition);

    return buffer;
}
