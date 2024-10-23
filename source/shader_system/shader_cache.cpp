#include "pch.h"

#include "shader_cache.h"

#include "utils/debug/assertion.h"
#include "utils/file/file.h"


static constexpr size_t AM_SHADER_CACHE_SUBMITION_PREALLOCATION_SIZE = 4 << 20;


static const fs::path AM_SHADER_SPIRV_EXTENSION = ".spv";


bool VulkanShaderCache::Load(const fs::path& shaderCacheFilepath) noexcept
{
    ReadBinaryFile(shaderCacheFilepath, m_cacheStorage);

    if (m_cacheStorage.empty()) {
        return false;
    }

    const uint8_t* pStorageBeginU8  = m_cacheStorage.data();
    const uint8_t* pStorageEndU8    = m_cacheStorage.data() + m_cacheStorage.size();

    const size_t cacheEntryCount = *((const uint32_t*)pStorageBeginU8);
    m_cacheLocations.reserve(cacheEntryCount);

    const uint8_t* pCacheEntry = pStorageBeginU8 + sizeof(uint32_t);
    while (pCacheEntry < pStorageEndU8) {
        uint32_t cacheEntrySize = 0;
        memcpy_s(&cacheEntrySize, sizeof(cacheEntrySize), pCacheEntry, pStorageEndU8 - pCacheEntry);

        pCacheEntry += sizeof(cacheEntrySize);

        ShaderIDProxy id;
        memcpy_s(&id, sizeof(id), pCacheEntry, pStorageEndU8 - pCacheEntry);

        pCacheEntry += sizeof(id);

        VulkanShaderCacheEntryLocation entryLocation;
        entryLocation.beginPosition = pCacheEntry - pStorageBeginU8;
        entryLocation.sizeInU8 = cacheEntrySize;

        m_cacheLocations[id] = entryLocation;

        pCacheEntry += cacheEntrySize;
    }

    // const size_t shaderCacheFilesCount = CalculateFilesCount(shaderCacheDirectory);

    // if (shaderCacheFilesCount == 0) {
    //     AM_LOG_GRAPHICS_API_WARN("There are no any shader cache files");
    //     return false;
    // }

    // m_cachedFilepathsToSPVFiles.reserve(shaderCacheFilesCount);
    
    // size_t shaderCacheSize = 0;

    // ForEachFileInDirectory(shaderCacheDirectory, [this, &shaderCacheSize](const fs::directory_entry& fileEntry)
    // {
    //     const fs::path& filepath = fileEntry.path();

    //     if (fs::file_size(filepath) <= sizeof(uint64_t)) {
    //         AM_LOG_GRAPHICS_API_WARN("{} size is less or equal to {}, so it's empty shader cache file. Skiped");
    //         return;
    //     }

    //     shaderCacheSize += fs::file_size(fileEntry.path());
    //     m_cachedFilepathsToSPVFiles.emplace_back(filepath);
    // });

    // m_cachedFilepathsToSPVFiles.shrink_to_fit();
    // const size_t validShaderCacheFilesCount = m_cachedFilepathsToSPVFiles.size();

    // // ShaderIDProxy goes first in shader cache file, so we need to calculate size of actual SPIR-V code
    // shaderCacheSize -= sizeof(ShaderIDProxy) * validShaderCacheFilesCount;

    // m_cacheStorage.resize(shaderCacheSize);
    // m_cacheLocations.reserve(validShaderCacheFilesCount);

    // ShaderIDProxy lastAddedIdProxy = {};

    // ForEachFileInDirectory(shaderCacheDirectory, [this, &lastAddedIdProxy](const fs::directory_entry& fileEntry)
    // {
    //     static constexpr size_t SHADER_ID_PROXY_SIZE = sizeof(ShaderIDProxy);

    //     const fs::path& filepath = fileEntry.path();

    //     const std::string filepathStr = filepath.string();
    //     AM_LOG_GRAPHICS_API_INFO("{}Reading {} shader cache{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, filepathStr.c_str(), AM_OUTPUT_COLOR_RESET_ASCII_CODE);

    //     const auto fileContentOpt = ReadBinaryFile(filepath);

    //     if (!fileContentOpt.has_value()) {
    //         return;
    //     }

    //     const std::vector<uint8_t>& fileContent = fileContentOpt.value();

    //     ShaderIDProxy idProxy = {};
    //     memcpy_s(&idProxy, SHADER_ID_PROXY_SIZE, fileContent.data(), SHADER_ID_PROXY_SIZE);

    //     VulkanShaderCacheEntryLocation insetionPosSize = {};

    //     if (!m_cacheLocations.empty()) {
    //         const VulkanShaderCacheEntryLocation& lastAddedLocation = m_cacheLocations[lastAddedIdProxy];
            
    //         insetionPosSize.beginPosition = lastAddedLocation.beginPosition + lastAddedLocation.sizeInU8;
    //         insetionPosSize.sizeInU8 = fileContent.size() - SHADER_ID_PROXY_SIZE;
    //     }

    //     lastAddedIdProxy = idProxy;
    //     m_cacheLocations[idProxy] = insetionPosSize;

    //     uint8_t* dst = m_cacheStorage.data() + insetionPosSize.beginPosition;
    //     const uint8_t* src = fileContent.data() + SHADER_ID_PROXY_SIZE;
    //     const size_t dstSize = m_cacheStorage.size() - insetionPosSize.beginPosition;
    //     const size_t srcSize = fileContent.size() - SHADER_ID_PROXY_SIZE;

    //     memcpy_s(dst, dstSize, src, srcSize);
    // });

    return true;
}


void VulkanShaderCache::Clear() noexcept
{
    *this = {};
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

    if (cacheLocation.beginPosition + cacheLocation.sizeInU8 >= m_cacheStorage.size()) {
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
    if (m_submitionCacheEntryCount == 0) {
        // First cache entry
        m_submitBuffer.reserve(AM_SHADER_CACHE_SUBMITION_PREALLOCATION_SIZE);
        
        // By the time of actual sumbition to file we will store m_submitionCacheEntryCount in this 4 bytes
        m_submitBuffer.resize(sizeof(uint32_t));
    }

    const size_t prevSubmitBufferSize = m_submitBuffer.size();
    const uint32_t cacheEntrySize         = sizeof(uint32_t) + sizeof(ShaderIDProxy) + shaderCompiledCode.size();

    m_submitBuffer.resize(prevSubmitBufferSize + cacheEntrySize);

    uint8_t* pCacheEntrySizeBuffer = m_submitBuffer.data() + prevSubmitBufferSize;
    memcpy_s(pCacheEntrySizeBuffer, cacheEntrySize, &cacheEntrySize, sizeof(uint32_t));

    ShaderIDProxy idProxy(id.Hash());

    uint8_t* pShaderHashIdBuffer = pCacheEntrySizeBuffer + sizeof(uint32_t);
    memcpy_s(pShaderHashIdBuffer, cacheEntrySize - sizeof(uint32_t), &idProxy, sizeof(ShaderIDProxy));

    uint8_t* pShaderCodeBuffer = pShaderHashIdBuffer + sizeof(ShaderIDProxy);
    memcpy_s(pShaderCodeBuffer, cacheEntrySize - sizeof(uint32_t) - sizeof(ShaderIDProxy), shaderCompiledCode.data(), shaderCompiledCode.size());

    ++m_submitionCacheEntryCount;
}


void VulkanShaderCache::Submit(const fs::path &shaderCacheFilepath) noexcept
{
    WriteBinaryFile(shaderCacheFilepath, m_submitBuffer.data(), m_submitBuffer.size());
    Clear();
}


VulkanShaderCompiledCodeBuffer VulkanShaderCache::GetShaderPrecompiledCode(const VulkanShaderCacheEntryLocation& location) const noexcept
{
    AM_ASSERT_GRAPHICS_API(location.sizeInU8 % sizeof(uint32_t) == 0, "Shader cache entry size is not multiple of sizeof(uint32_t)");

    VulkanShaderCompiledCodeBuffer buffer = {};

    buffer.sizeInU32    = location.sizeInU8 / sizeof(uint32_t);
    buffer.pCode        = (const uint32_t*)(m_cacheStorage.data() + location.beginPosition);

    return buffer;
}
