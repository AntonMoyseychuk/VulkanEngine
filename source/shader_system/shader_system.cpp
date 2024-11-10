#include "pch.h"
#include "shader_system.h"

#include "path_system/path_system.h"

#include "utils/data_structures/strid.h"

#include "utils/debug/assertion.h"
#include "utils/file/file.h"
#include "utils/json/json.h"

#include <shaderc/shaderc.hpp>


#if defined(AM_DEBUG)
    static constexpr ShaderID::OptimizationLevel AM_SHADERC_OPTIMIZATION_LEVEL = ShaderID::OPTIMIZATION_LEVEL_NONE; 
#elif defined(AM_RELEASE)
    static constexpr ShaderID::OptimizationLevel AM_SHADERC_OPTIMIZATION_LEVEL = ShaderID::OPTIMIZATION_LEVEL_SPEED; 
#endif


static constexpr const char* JSON_SHADER_SETUP_DEFINES_FIELD_NAME           = "defines";
static constexpr const char* JSON_SHADER_SETUP_DEFINES_CONDITION_FIELD_NAME = "condition";
static constexpr const char* JSON_SHADER_SETUP_DEFINES_TYPE_FIELD_NAME      = "type";
static constexpr const char* JSON_SHADER_SETUP_DEFINES_TYPE_VERTEX          = "vs";
static constexpr const char* JSON_SHADER_SETUP_DEFINES_TYPE_PIXEL           = "ps";

static const fs::path AM_VERTEX_SHADER_EXTENSIONS[] = { ".vs", ".vsh", ".vert", ".glsl" };
static const fs::path AM_PIXEL_SHADER_EXTENSIONS[]  = { ".ps", ".fs", ".psh", ".frag", ".glsl" };


static constexpr uint32_t AM_VERTEX_SHADER_MASK = 0x1;
static constexpr uint32_t AM_PIXEL_SHADER_MASK  = 0x2;


static shaderc::Compiler g_shadercCompiler;


struct VulkanShaderGroupFilepaths
{
    ds::StrID setupFilepath;
    ds::StrID vsFilepath;
    ds::StrID psFilepath;
};


struct VulkanShaderDefine
{
    bool IsVertex() const noexcept { return (shaderTypeMask & AM_VERTEX_SHADER_MASK) != 0; }
    bool IsPixel() const noexcept { return (shaderTypeMask & AM_PIXEL_SHADER_MASK) != 0; }

    std::string condition;
    std::string name;
    uint32_t    shaderTypeMask = 0;
};


class VulkanShaderGroupSetup
{
public:
    VulkanShaderGroupSetup(const fs::path& jsonFilepath);

    size_t GetVSDefinesCombinationsCount() const noexcept { return GetShaderDefineCombinationsCount(m_vsDefinesPtrs.size()); }
    size_t GetPSDefinesCombinationsCount() const noexcept { return GetShaderDefineCombinationsCount(m_psDefinesPtrs.size()); }

    const std::vector<const VulkanShaderDefine*>& GetVSDefines() const noexcept { return m_vsDefinesPtrs; }
    const std::vector<const VulkanShaderDefine*>& GetPSDefines() const noexcept { return m_psDefinesPtrs; }

    const std::vector<VulkanShaderDefine>& GetDefines() const noexcept { return m_defines; }

public:
    static VulkanShaderGroupSetup ParseJSON(const fs::path& jsonFilepath) noexcept;

private:
    VulkanShaderGroupSetup() = default;

    static size_t GetShaderDefineCombinationsCount(size_t definesCount) noexcept { return 1 + (1 + definesCount) * definesCount / 2; }

private:
    std::vector<VulkanShaderDefine> m_defines;

    std::vector<const VulkanShaderDefine*> m_vsDefinesPtrs;
    std::vector<const VulkanShaderDefine*> m_psDefinesPtrs;
};


struct VulkanShaderBuildInfo
{
    bool IsValid() const noexcept 
    { 
        return pShaderId && pShaderId->IsHashValid() && (kind == shaderc_vertex_shader || kind == shaderc_fragment_shader);
    }

    const ShaderID* pShaderId = nullptr;
    shaderc_shader_kind kind;
    shaderc::CompileOptions compileOptions;
};


static bool IsVertexShaderFile(const fs::path& filepath) noexcept
{
    const fs::path fileExtension = filepath.extension();

    for (const fs::path& ext : AM_VERTEX_SHADER_EXTENSIONS) {
        if (ext == fileExtension) {
            return true;
        }
    }

    return false;
}


static shaderc_optimization_level ShaderIdOptimizationLevelToShadercLevel(ShaderID::OptimizationLevel level) noexcept
{
    AM_ASSERT_GRAPHICS_API(level < ShaderID::OPTIMIZATION_LEVEL_COUNT, "Invalid optimization level ({})", static_cast<uint32_t>(level));
    
    switch(level) {
    case ShaderID::OPTIMIZATION_LEVEL_NONE:
        return shaderc_optimization_level_zero;    
    case ShaderID::OPTIMIZATION_LEVEL_SPEED:
        return shaderc_optimization_level_performance;
    case ShaderID::OPTIMIZATION_LEVEL_SIZE:
        return shaderc_optimization_level_size;
    default:
        return shaderc_optimization_level_zero;
    }
}


static bool IsPixelShaderFile(const fs::path& filepath) noexcept
{
    const fs::path fileExtension = filepath.extension();

    for (const fs::path& ext : AM_PIXEL_SHADER_EXTENSIONS) {
        if (ext == fileExtension) {
            return true;
        }
    }

    return false;
}


static bool IsShaderGroupSetupFile(const fs::path& filepath) noexcept
{
    return filepath.extension() == ".json";
}


static std::optional<shaderc_shader_kind> GetShaderCShaderKind(const ShaderID& shaderId) noexcept
{
    AM_ASSERT_GRAPHICS_API(shaderId.IsHashValid(), "shaderId is invalid");

    const fs::path shaderFilepath = *shaderId.GetFilepath().String();

    if (IsVertexShaderFile(shaderFilepath)) {
        return shaderc_vertex_shader;
    } else if (IsPixelShaderFile(shaderFilepath)) {
        return shaderc_fragment_shader;
    }

    return std::nullopt;
}


static bool PreprocessShader(std::vector<uint8_t>& buffer, const VulkanShaderBuildInfo& buildInfo) noexcept
{
    AM_ASSERT_GRAPHICS_API(buildInfo.IsValid(), "buildInfo is invalid");

    const ShaderID& shaderId = *buildInfo.pShaderId;

    std::vector<uint8_t>& sourceCode = buffer;

    const char* shaderFilepath = shaderId.GetFilepath().CStr();

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Preprocessing '{}'..."), shaderFilepath);

    shaderc::PreprocessedSourceCompilationResult result = g_shadercCompiler.PreprocessGlsl(reinterpret_cast<char*>(sourceCode.data()), sourceCode.size(), 
        buildInfo.kind, shaderFilepath, buildInfo.compileOptions);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        AM_LOG_GRAPHICS_API_ERROR("Shader {} preprocess error:\n{}", shaderFilepath, result.GetErrorMessage().c_str());
        return false;
    }

    const char* preprocessedSourceCode = result.cbegin();
    const size_t newSourceCodeSize = (uintptr_t)result.cend() - (uintptr_t)preprocessedSourceCode;
    
    sourceCode.resize(newSourceCodeSize);
    memcpy_s(sourceCode.data(), sourceCode.size(), preprocessedSourceCode, newSourceCodeSize);

#if defined(AM_LOGGING_ENABLED)
    std::string output(reinterpret_cast<char*>(sourceCode.data()), sourceCode.size());
    AM_LOG_GRAPHICS_API_INFO("Preprocessed GLSL source code ({}):\n{}", shaderFilepath, output.c_str());
#endif

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Preprocessing '{}' finished successfuly"), shaderFilepath);

    return true;
}


static bool CompileShaderToAssembly(std::vector<uint8_t>& buffer, const VulkanShaderBuildInfo& buildInfo) noexcept
{
    AM_ASSERT_GRAPHICS_API(buildInfo.IsValid(), "buildInfo is invalid");

    const ShaderID& shaderId = *buildInfo.pShaderId;

    std::vector<uint8_t>& preproccessedSourceCode = buffer;

    const char* shaderFilepath = shaderId.GetFilepath().CStr();

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Compling '{}' to assembly..."), shaderFilepath);

    shaderc::AssemblyCompilationResult result = g_shadercCompiler.CompileGlslToSpvAssembly(reinterpret_cast<char*>(preproccessedSourceCode.data()), preproccessedSourceCode.size(),
        buildInfo.kind, shaderFilepath, buildInfo.compileOptions);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        AM_LOG_GRAPHICS_API_ERROR("Shader {} compiling to assembly error:\n{}", shaderFilepath, result.GetErrorMessage().c_str());
        return false;
    }

    const char* assemlyCodeRaw = result.cbegin();
    const size_t assemblyCodeSize = (uintptr_t)result.cend() - (uintptr_t)assemlyCodeRaw;
    
    std::vector<uint8_t>& assemblyCode = preproccessedSourceCode;

    assemblyCode.resize(assemblyCodeSize);
    memcpy_s(reinterpret_cast<char*>(assemblyCode.data()), assemblyCode.size(), assemlyCodeRaw, assemblyCodeSize);

#if defined(AM_LOGGING_ENABLED)
    std::string output(reinterpret_cast<char*>(assemblyCode.data()), assemblyCode.size());
    AM_LOG_GRAPHICS_API_INFO("SPIR-V assembly code ({}):\n{}", shaderFilepath, output.c_str());
#endif

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Compling '{}' to assembly finished"), shaderFilepath);

    return true;
}


static bool AssembleShaderToSPIRV(std::vector<uint8_t>& buffer, const VulkanShaderBuildInfo& buildInfo) noexcept
{
    AM_ASSERT_GRAPHICS_API(buildInfo.IsValid(), "buildInfo is invalid");

    const ShaderID& shaderId = *buildInfo.pShaderId;

    std::vector<uint8_t>& assemblyCode = buffer;

    const char* shaderFilepath = shaderId.GetFilepath().CStr();

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Assembling '{}' to SPIR-V..."), shaderFilepath);

    shaderc::SpvCompilationResult result = g_shadercCompiler.AssembleToSpv(
        reinterpret_cast<char*>(assemblyCode.data()), assemblyCode.size(), buildInfo.compileOptions);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        AM_LOG_GRAPHICS_API_ERROR("Shader {} assembling to SPIR-V error:\n{}", shaderFilepath, result.GetErrorMessage().c_str());
        return false;
    }

    const uint32_t* spirvCodeRaw = result.cbegin();
    const size_t spirvCodeSizeInU32 = result.cend() - result.cbegin();
    const size_t spirvCodeSizeInU8 = spirvCodeSizeInU32 * sizeof(uint32_t);

    buffer.resize(spirvCodeSizeInU8);
    memcpy_s(buffer.data(), spirvCodeSizeInU8, spirvCodeRaw, spirvCodeSizeInU8);

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Assembling '{}' to SPIR-V finished"), shaderFilepath);

    return true;
}


static std::vector<uint8_t> BuildSPIRVCodeFormFile(const VulkanShaderGroupSetup& setup, const ShaderID& shaderId) noexcept
{
    AM_ASSERT_GRAPHICS_API(shaderId.IsHashValid(), "Invalid shaderId");

    VulkanShaderBuildInfo buildInfo = {};
    buildInfo.pShaderId = &shaderId;
    
    const auto shaderKind = GetShaderCShaderKind(shaderId);
    AM_ASSERT_GRAPHICS_API(shaderKind.has_value(), "Invalid shader kind");
    buildInfo.kind = shaderKind.value();

    buildInfo.compileOptions.SetOptimizationLevel(ShaderIdOptimizationLevelToShadercLevel(shaderId.GetOptimizationLevel()));

    const auto FillCompileOptionsDefines = [&shaderId, &buildInfo](const std::vector<const VulkanShaderDefine*>& defines)
    {
        for (size_t i = 0; i < defines.size(); ++i) {
            if (shaderId.IsDefineBit(i)) {
                buildInfo.compileOptions.AddMacroDefinition(defines[i]->name);
            }
        }
    };
    
    switch (buildInfo.kind) {
        case shaderc_vertex_shader:
            FillCompileOptionsDefines(setup.GetVSDefines());
            break;

        case shaderc_fragment_shader:
            FillCompileOptionsDefines(setup.GetPSDefines());
            break;

        default:
            AM_ASSERT_GRAPHICS_API_FAIL("Invalid shader kind");
            break;
    }

    std::vector<uint8_t> buffer;
    ReadBinaryFile(shaderId.GetFilepath().CStr(), buffer);

    if (buffer.empty()) {
        return {};
    }

    if (!PreprocessShader(buffer, buildInfo)) {
        return {};
    }

    if (!CompileShaderToAssembly(buffer, buildInfo)) {
        return {};
    }

    if (!AssembleShaderToSPIRV(buffer, buildInfo)) {
        return {};
    }

    return buffer;
}


static VkShaderModule CreateVulkanShaderModule(VkDevice pLogicalDevice, const uint32_t* pCode, size_t codeSize) noexcept
{
    AM_ASSERT_GRAPHICS_API(pLogicalDevice != VK_NULL_HANDLE, "Invalid Vulkan logical device handle");
    AM_ASSERT_GRAPHICS_API(pCode != nullptr, "pCode is nullptr");

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode    = pCode;

    VkShaderModule pModule;
    if (vkCreateShaderModule(pLogicalDevice, &createInfo, nullptr, &pModule) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API_FAIL("Vulkan shader module creation failed");
        return VK_NULL_HANDLE;
    }

    return pModule;
}


static VkShaderModule CreateVulkanShaderModule(VkDevice pLogicalDevice, const VulkanShaderGroupSetup& setup, const ShaderID &id) noexcept
{
    std::vector<uint8_t> compiledCode = BuildSPIRVCodeFormFile(setup, id);
    return CreateVulkanShaderModule(pLogicalDevice, (const uint32_t*)compiledCode.data(), compiledCode.size());
}


static std::vector<VulkanShaderGroupFilepaths> GetShaderGroupFilepathsList(const fs::path& shadersRootDir) noexcept
{
    const size_t groupsCount = CalculateDirectoriesCount(shadersRootDir);

    if (groupsCount == 0) {
        AM_LOG_ERROR("Failed to find any shader source files");
        return {};
    }

    std::vector<VulkanShaderGroupFilepaths> result;
    result.reserve(groupsCount);

    ForEachDirectory(shadersRootDir, [&result](const fs::directory_entry& dirEntry)
    {
        VulkanShaderGroupFilepaths pathGroup;

        ForEachFile(dirEntry.path(), [&pathGroup](const fs::directory_entry& fileEntry)
        {
            const fs::path& filepath = fileEntry.path();

            if (IsShaderGroupSetupFile(filepath)) {
                pathGroup.setupFilepath = filepath.string();
            } else if (IsVertexShaderFile(filepath)) {
                pathGroup.vsFilepath = filepath.string();
            } else if (IsPixelShaderFile(filepath)) {
                pathGroup.psFilepath = filepath.string();
            } else {
                AM_ASSERT_FAIL("Invalid shader related file type");
            }
        }, 0);

        result.emplace_back(pathGroup);
    }, 0);

    return result;
}


VulkanShaderSystem& VulkanShaderSystem::Instance() noexcept
{
    AM_ASSERT(s_pShaderSysInstance != nullptr, "Vulkan shader system is not initialized, call VulkanShaderSystem::Init(...) first");
    
    return *s_pShaderSysInstance;
}


bool VulkanShaderSystem::Init(VkDevice pLogicalDevice) noexcept
{
    if (IsInitialized()) {
        AM_LOG_WARN("VulkanShaderSystem is already initialized");
        return true;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing VulkanShaderSystem..."));

    if (pLogicalDevice == VK_NULL_HANDLE) {
        AM_ASSERT_GRAPHICS_API_FAIL("Vulkan logical device is VK_NULL_HANDLE");
        return false;
    }

    s_pLogicalDevice = pLogicalDevice;

    s_pShaderSysInstance = std::unique_ptr<VulkanShaderSystem>(new VulkanShaderSystem);
    if (!s_pShaderSysInstance) {
        AM_ASSERT_GRAPHICS_API_FAIL("Failed to allocate VulkanShaderSystem");
        return false;
    }

    s_pShaderSysInstance->InitializeShaders();

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "VulkanShaderSystem initialization finished"));
    return true;
}


void VulkanShaderSystem::Terminate() noexcept
{
    s_pShaderSysInstance = nullptr;
}


bool VulkanShaderSystem::IsInitialized() noexcept
{
    return IsInstanceInitialized() && IsVulkanLogicalDeviceValid();
}


void VulkanShaderSystem::RecompileShaders() noexcept
{
    CompileShaders(true);
}


bool VulkanShaderSystem::IsInstanceInitialized() noexcept
{
    return s_pShaderSysInstance != nullptr;
}


bool VulkanShaderSystem::IsVulkanLogicalDeviceValid() noexcept
{
    return s_pLogicalDevice != VK_NULL_HANDLE;
}


VulkanShaderSystem::VulkanShaderSystem()
    : m_pShaderCache(std::make_unique<VulkanShaderCache>())
{
    AM_ASSERT_GRAPHICS_API(m_pShaderCache != nullptr, "Failed to allocate Vulkan shader cache");
}


bool VulkanShaderSystem::IsShaderCacheInitialized() const noexcept
{
    return m_pShaderCache != nullptr;
}


VulkanShaderSystem::~VulkanShaderSystem()
{
    ClearVulkanShaderModules();
}


bool VulkanShaderSystem::InitializeShaders() noexcept
{
    AM_ASSERT(IsShaderCacheInitialized(), "Vulkan shader cache is not initialized");

    const bool isShaderCacheEmpty = !m_pShaderCache->Load(PathSystem::GetProjectShaderCacheFilepath());
    const bool shouldForceCompileShaders = isShaderCacheEmpty;

    CompileShaders(shouldForceCompileShaders);

    return true;
}


void VulkanShaderSystem::ClearVulkanShaderModules() noexcept
{
    AM_ASSERT_GRAPHICS_API(IsVulkanLogicalDeviceValid(), "Reference to invalid Vulkan logical device inside {}", __FUNCTION__);

    for (auto& [shaderId, pVkModule] : m_shaderModules) {
        vkDestroyShaderModule(s_pLogicalDevice, pVkModule, nullptr);
        pVkModule = VK_NULL_HANDLE;
    }

    m_shaderModules.clear();
}

void VulkanShaderSystem::CompileShaders(bool forceRecompile) noexcept
{
    std::vector<VulkanShaderGroupFilepaths> shaderGroupFilepathsList = GetShaderGroupFilepathsList(PathSystem::GetProjectShadersSourceCodeDirectory());
    
    std::vector<VulkanShaderGroupSetup> setups;
    setups.reserve(shaderGroupFilepathsList.size());
    
    size_t totalShaderCombinations = 0;

    for (const VulkanShaderGroupFilepaths& groupFilepaths : shaderGroupFilepathsList) {
        VulkanShaderGroupSetup setup = VulkanShaderGroupSetup::ParseJSON(fs::path(groupFilepaths.setupFilepath.CStr()));
        
        totalShaderCombinations += setup.GetVSDefinesCombinationsCount() + setup.GetPSDefinesCombinationsCount();

        setups.emplace_back(setup);
    }

    ClearVulkanShaderModules();

    m_shaderModules.reserve(totalShaderCombinations);

    const auto CreateAllCombinationsShaderModules = [this](const VulkanShaderGroupSetup& setup, 
        const std::vector<const VulkanShaderDefine*>& defines, ds::StrID shaderFilepath, bool forceRecompile) -> bool
    {
        ShaderID shaderId(shaderFilepath, {}, AM_SHADERC_OPTIMIZATION_LEVEL);

        bool newShaderCacheEntry = false;

        if (forceRecompile || !LoadAndAddShaderModule(shaderId)) {
            newShaderCacheEntry = BuildAndAddShaderModule(&setup, shaderId) || newShaderCacheEntry;
        }

        for (size_t j = 0; j < defines.size(); ++j) {
            shaderId.ClearBits();

            for (size_t k = j; k < defines.size(); ++k) {
                shaderId.SetDefineBit(k);

                if (forceRecompile || !LoadAndAddShaderModule(shaderId)) {
                    newShaderCacheEntry = BuildAndAddShaderModule(&setup, shaderId) || newShaderCacheEntry;
                }
            }
        }

        return newShaderCacheEntry;
    };

    bool needToSubmitShaderCache = false;

    for (size_t i = 0; i < setups.size(); ++i) {
        const VulkanShaderGroupSetup& setup = setups[i];

        const bool newVsCombinations = CreateAllCombinationsShaderModules(setup, setup.GetVSDefines(), shaderGroupFilepathsList[i].vsFilepath, forceRecompile);
        const bool newPsCombinations = CreateAllCombinationsShaderModules(setup, setup.GetPSDefines(), shaderGroupFilepathsList[i].psFilepath, forceRecompile);

        needToSubmitShaderCache = needToSubmitShaderCache || newVsCombinations || newPsCombinations;
    }

    if (needToSubmitShaderCache) {
        m_pShaderCache->Submit(PathSystem::GetProjectShaderCacheFilepath());
    }
}


bool VulkanShaderSystem::BuildAndAddShaderModule(const void* pSetup, const ShaderID &shaderId) noexcept
{
    const VulkanShaderGroupSetup& setup = *(const VulkanShaderGroupSetup*)pSetup;

    const std::vector<uint8_t> spirvCode = BuildSPIRVCodeFormFile(setup, shaderId);

    VkShaderModule pShaderModule = CreateVulkanShaderModule(s_pLogicalDevice, (const uint32_t*)spirvCode.data(), spirvCode.size());

    if (pShaderModule == VK_NULL_HANDLE) {
        return false;
    }

    m_shaderModules[ShaderIDProxy(shaderId)] = pShaderModule;

    m_pShaderCache->AddCacheEntryToSubmitBuffer(shaderId, spirvCode);

    return true;
}


bool VulkanShaderSystem::LoadAndAddShaderModule(const ShaderID &shaderId) noexcept
{
    if (!m_pShaderCache->Contains(shaderId)) {
        AM_LOG_GRAPHICS_API_WARN("Failed to load {} from shader cache", shaderId.GetFilepath().CStr());
        return false;
    }

    const VulkanShaderCompiledCodeBuffer shaderCacheEntry = m_pShaderCache->GetShaderPrecompiledCode(shaderId);
    
    VkShaderModule pShaderModule = CreateVulkanShaderModule(s_pLogicalDevice, shaderCacheEntry.pCode, 
        shaderCacheEntry.sizeInU32 * sizeof(uint32_t));

    if (pShaderModule == VK_NULL_HANDLE) {
        return false;
    }

    m_shaderModules[ShaderIDProxy(shaderCacheEntry.hash)] = pShaderModule;
    return true;
}


VulkanShaderGroupSetup::VulkanShaderGroupSetup(const fs::path &jsonFilepath)
{
    const auto setupJsonConf = amjson::ParseJson(jsonFilepath);
    AM_ASSERT(setupJsonConf.has_value(), "VulkanShaderGroupSetup Json parsing error");

    const nlohmann::json& setupJson = setupJsonConf.value();

    const nlohmann::json& definesJson = amjson::GetJsonSubNode(setupJson, JSON_SHADER_SETUP_DEFINES_FIELD_NAME);
    const size_t definesCount = definesJson.size();

    if (!definesCount) {
        AM_LOG_INFO("No defines in {} detected", jsonFilepath.string().c_str());
        return;
    }

    m_defines.reserve(definesCount);
    m_vsDefinesPtrs.reserve(definesCount);
    m_psDefinesPtrs.reserve(definesCount);

    for (const auto& [defineName, defineDescJson] : definesJson.items()) {
        VulkanShaderDefine define = {};
        
        define.name = defineName;
        amjson::GetJsonSubNode(defineDescJson, JSON_SHADER_SETUP_DEFINES_CONDITION_FIELD_NAME).get_to<std::string>(define.condition);
        
        bool isVertex = false, isPixel = false;

        for (const std::string& type : amjson::ParseJsonSubNodeToArray<std::string>(defineDescJson, JSON_SHADER_SETUP_DEFINES_TYPE_FIELD_NAME)) {
            if (type == JSON_SHADER_SETUP_DEFINES_TYPE_VERTEX) {
                define.shaderTypeMask |= AM_VERTEX_SHADER_MASK;
                isVertex = true;
            } else if (type == JSON_SHADER_SETUP_DEFINES_TYPE_PIXEL) {
                define.shaderTypeMask |= AM_PIXEL_SHADER_MASK;
                isPixel = true;
            } else {
                AM_ASSERT_FAIL("Invalid shader setup type");
            }
        }

        m_defines.emplace_back(define);
        
        if (isVertex) {
            m_vsDefinesPtrs.emplace_back(&(*m_defines.rbegin()));
        }

        if (isPixel) {
            m_psDefinesPtrs.emplace_back(&(*m_defines.rbegin()));
        }
    }
}


VulkanShaderGroupSetup VulkanShaderGroupSetup::ParseJSON(const fs::path &jsonFilepath) noexcept
{
    return VulkanShaderGroupSetup(jsonFilepath);
}
