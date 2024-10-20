#include "pch.h"

#include "shader_cache.h"

#include "utils/debug/assertion.h"
#include "utils/file/file.h"


static const fs::path AM_SHADER_SPIRV_EXTENSION = ".spv";


bool VulkanShaderCache::Load(const fs::path& shaderCacheDirectory) noexcept
{
    const size_t shaderCacheFilesCount = CalculateFilesCountInDirectory(shaderCacheDirectory);

    if (shaderCacheFilesCount == 0) {
        AM_LOG_GRAPHICS_API_WARN("There are no any shader cache files");
        return false;
    }

    m_cachedFilepathsToSPVFiles.reserve(shaderCacheFilesCount);
    
    size_t shaderCacheSize = 0;

    ForEachFileInDirectory(shaderCacheDirectory, [this, &shaderCacheSize](const fs::directory_entry& fileEntry)
    {
        const fs::path& filepath = fileEntry.path();

        if (fs::file_size(filepath) <= sizeof(uint64_t)) {
            AM_LOG_GRAPHICS_API_WARN("{} size is less or equal to {}, so it's empty shader cache file. Skiped");
            return;
        }

        shaderCacheSize += fs::file_size(fileEntry.path());
        m_cachedFilepathsToSPVFiles.emplace_back(filepath);
    });

    m_cachedFilepathsToSPVFiles.shrink_to_fit();
    const size_t validShaderCacheFilesCount = m_cachedFilepathsToSPVFiles.size();

    // ShaderIDProxy goes first in shader cache file, so we need to calculate size of actual SPIR-V code
    shaderCacheSize -= sizeof(ShaderIDProxy) * validShaderCacheFilesCount;

    m_cacheStorage.resize(shaderCacheSize);
    m_cacheLocations.reserve(validShaderCacheFilesCount);

    ShaderIDProxy lastAddedIdProxy = {};

    ForEachFileInDirectory(shaderCacheDirectory, [this, &lastAddedIdProxy](const fs::directory_entry& fileEntry)
    {
        static constexpr size_t SHADER_ID_PROXY_SIZE = sizeof(ShaderIDProxy);

        const fs::path& filepath = fileEntry.path();

        const std::string filepathStr = filepath.string();
        AM_LOG_GRAPHICS_API_INFO("{}Reading {} shader cache{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, filepathStr.c_str(), AM_OUTPUT_COLOR_RESET_ASCII_CODE);

        const auto fileContentOpt = ReadBinaryFile(filepath);

        if (!fileContentOpt.has_value()) {
            return;
        }

        const std::vector<uint8_t>& fileContent = fileContentOpt.value();

        ShaderIDProxy idProxy = {};
        memcpy_s(&idProxy, SHADER_ID_PROXY_SIZE, fileContent.data(), SHADER_ID_PROXY_SIZE);

        VulkanShaderCachePosSize insetionPosSize = {};

        if (!m_cacheLocations.empty()) {
            const VulkanShaderCachePosSize& lastAddedLocation = m_cacheLocations[lastAddedIdProxy];
            
            insetionPosSize.beginPosition = lastAddedLocation.beginPosition + lastAddedLocation.sizeInU8;
            insetionPosSize.sizeInU8 = fileContent.size() - SHADER_ID_PROXY_SIZE;
        }

        lastAddedIdProxy = idProxy;
        m_cacheLocations[idProxy] = insetionPosSize;

        uint8_t* dst = m_cacheStorage.data() + insetionPosSize.beginPosition;
        const uint8_t* src = fileContent.data() + SHADER_ID_PROXY_SIZE;
        const size_t dstSize = m_cacheStorage.size() - insetionPosSize.beginPosition;
        const size_t srcSize = fileContent.size() - SHADER_ID_PROXY_SIZE;

        memcpy_s(dst, dstSize, src, srcSize);
    });

    return true;
}


void VulkanShaderCache::Terminate() noexcept
{
    *this = {};
}


VulkanShaderCompiledCodeBuffer VulkanShaderCache::GetShaderPrecompiledCode(const ShaderID & id) const noexcept
{
    ShaderIDProxy idProxy;
    idProxy.hash = id.Hash();

    const auto codeLocation = m_cacheLocations.find(idProxy);

    if (codeLocation == m_cacheLocations.cend()) {
        AM_ASSERT_GRAPHICS_API(false, "Invalid shader id");
        return {};
    }

    const VulkanShaderCachePosSize& codePosSize = codeLocation->second;

    if (codePosSize.beginPosition + codePosSize.sizeInU8 >= m_cacheStorage.size()) {
        AM_ASSERT_GRAPHICS_API(false, "Invalid shader cache buffer position + size");
        return {};
    }

    VulkanShaderCompiledCodeBuffer codeBuffer = {};
    codeBuffer.pCode = reinterpret_cast<const uint32_t*>(&m_cacheStorage[codePosSize.beginPosition]);
    
    if (codePosSize.sizeInU8 % sizeof(uint32_t) != 0) {
        AM_ASSERT_GRAPHICS_API(false, "SPIR-V code size must be multiple of sizeof(uint32_t)");
        return {};
    }

    codeBuffer.sizeInU32 = codePosSize.sizeInU8 / sizeof(uint32_t);

    return codeBuffer;
}


void VulkanShaderCache::Store(const fs::path& shaderCacheDirectory, const ShaderID& id, const std::vector<uint8_t>& shaderCompiledCode) const noexcept
{
    AM_ASSERT_GRAPHICS_API(id.IsHashValid(), "Invalid shader id hash");
    AM_ASSERT_GRAPHICS_API(id.GetFilepath().String() != nullptr, "Shader id filepath is nullptr");

    const fs::path shaderSourceFilepath(*id.GetFilepath().String());
    
    const uint64_t shaderHash = id.Hash();
    
    const fs::path shaderCacheFilePath = shaderCacheDirectory 
        / std::to_string(shaderHash) / shaderSourceFilepath.filename() / AM_SHADER_SPIRV_EXTENSION;

    std::vector<uint8_t> data(sizeof(shaderHash) + shaderCompiledCode.size());
    memcpy_s(data.data(), data.size(), &shaderHash, sizeof(shaderHash));
    memcpy_s(data.data() + sizeof(shaderHash), data.size() - sizeof(shaderHash), shaderCompiledCode.data(), shaderCompiledCode.size());

    WriteBinaryFile(shaderCacheFilePath, data.data(), data.size());
}


uint64_t VulkanShaderCache::ShaderIDProxyHasher::operator()(const ShaderIDProxy & proxy) const noexcept
{
    AM_ASSERT_GRAPHICS_API(proxy.IsValid(), "Invalid ShaderIDProxy");
            
    return proxy.hash;
}
