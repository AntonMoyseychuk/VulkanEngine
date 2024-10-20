#include "pch.h"
#include "shader_system.h"

#include "config.h"

#include "utils/data_structures/strid.h"

#include "utils/debug/assertion.h"
#include "utils/file/file.h"
#include "utils/json/json.h"


static const fs::path AM_VERTEX_SHADER_EXTENSIONS[] = { ".vs", ".vsh", ".vert", ".glsl" };
static const fs::path AM_PIXEL_SHADER_EXTENSIONS[] = { ".ps", ".fs", ".psh", ".frag", ".glsl" };
static const fs::path AM_SHADER_CONFIG_EXTENSION = ".json";


static constexpr const char* JSON_SHADER_CONFIG_DEFINES_FIELD_NAME = "defines";


struct VulkanShaderIntermediateData
{
    std::vector<char>       code;
    std::vector<uint32_t>   spirvCode;

    shaderc::CompileOptions compileOptions;
    shaderc_shader_kind     kind;
            
    std::string             filename;
};


static bool IsVertexShaderFileCorrectExtension(const fs::path& extension) noexcept
{
    for (const fs::path& ext : AM_VERTEX_SHADER_EXTENSIONS) {
        if (ext == extension) {
            return true;
        }
    }

    return false;
}


static bool IsPixelShaderFileCorrectExtension(const fs::path& extension) noexcept
{
    for (const fs::path& ext : AM_PIXEL_SHADER_EXTENSIONS) {
        if (ext == extension) {
            return true;
        }
    }

    return false;
}


static bool IsShaderGroupConfigFile(const fs::path& filepath) noexcept
{
    return filepath.extension() == AM_SHADER_CONFIG_EXTENSION;
}


static std::optional<VulkanShaderGroupConfigInfo> GetVulkanShaderGroupConfigInfo(const fs::path& pathToConfigJson) noexcept
{
    const std::optional<nlohmann::json> configJsonOpt = amjson::ParseJson(pathToConfigJson);
    if (!configJsonOpt.has_value()) {
        AM_ASSERT(false, "Failed to parse '{}' shader config file", pathToConfigJson.string().c_str());
        return {};
    }

    const nlohmann::json& configJson = configJsonOpt.value();

    VulkanShaderGroupConfigInfo info = {};
    info.defines = amjson::ParseJsonSubNodeToArray<std::string>(configJson, JSON_SHADER_CONFIG_DEFINES_FIELD_NAME);

    return info;
}


static std::optional<VulkanShaderIntermediateData> CreateVulkanShaderModuleIntermediateData(const VulkanShaderModuleIntermediateDataConfigInfo& configInfo) noexcept
{
    AM_ASSERT_GRAPHICS_API(configInfo.pFilepath, "configInfo.pFilepath is nullptr");
    AM_ASSERT_GRAPHICS_API(configInfo.pGroupConfigInfo, "configInfo.pGroupConfigInfo is nullptr");

    std::optional<std::vector<char>> sourceCodeOpt = ReadFile(*configInfo.pFilepath);
            
    if(!sourceCodeOpt.has_value()) {
        AM_LOG_GRAPHICS_API_ERROR("Shader compilation error: '{}' invalid content. Skiped", configInfo.pFilepath->filename().string().c_str());
        return {};
    }

    std::vector<char>& sourceCode = sourceCodeOpt.value();

    VulkanShaderIntermediateData data = {};
    data.code     = std::move(sourceCode);
    data.kind     = configInfo.kind == VulkanShaderKind_VERTEX ? shaderc_vertex_shader : shaderc_fragment_shader;
    data.filename = configInfo.pFilepath->filename().string();
    
#if defined(AM_DEBUG)
    data.compileOptions.SetOptimizationLevel(shaderc_optimization_level_zero);
#else
    data.compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif

    for (const std::string& define : configInfo.pGroupConfigInfo->defines) {
        data.compileOptions.AddMacroDefinition(define);
    }

    return data;
}


VulkanShaderSystem& VulkanShaderSystem::Instance() noexcept
{
    AM_ASSERT(s_pShaderSysInstace != nullptr, "Vulkan shader system is not initialized, call VulkanShaderSystem::Init(...) first");
    
    return *s_pShaderSysInstace;
}


bool VulkanShaderSystem::Init(VkDevice pLogicalDevice) noexcept
{
    if (IsInitialized()) {
        AM_LOG_WARN("VulkanShaderSystem is already initialized");
        return true;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing VulkanShaderSystem..."));

    if (pLogicalDevice == VK_NULL_HANDLE) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan logical device is VK_NULL_HANDLE");
        return false;
    }

    s_pLogicalDevice = pLogicalDevice;

    s_pShaderSysInstace = std::unique_ptr<VulkanShaderSystem>(new VulkanShaderSystem);
    if (!s_pShaderSysInstace) {
        AM_ASSERT_GRAPHICS_API(false, "Failed to allocate VulkanShaderSystem");
        return false;
    }

    s_pShaderSysInstace->RecompileShaders();

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "VulkanShaderSystem initialization finished"));
    return true;
}


void VulkanShaderSystem::Terminate() noexcept
{
    s_pShaderSysInstace = nullptr;
}


bool VulkanShaderSystem::IsInitialized() noexcept
{
    return IsInstanceInitialized() && IsVulkanLogicalDeviceValid();
}


void VulkanShaderSystem::RecompileShaders() noexcept
{
    CompileShaders();
}


bool VulkanShaderSystem::IsInstanceInitialized() noexcept
{
    return s_pShaderSysInstace != nullptr;
}


bool VulkanShaderSystem::IsVulkanLogicalDeviceValid() noexcept
{
    return s_pLogicalDevice != VK_NULL_HANDLE;
}


VulkanShaderSystem::VulkanShaderSystem()
{
    
}


VulkanShaderSystem::~VulkanShaderSystem()
{
    ClearShaderModuleGroups();
}


void VulkanShaderSystem::ClearShaderModuleGroups() noexcept
{
    AM_ASSERT_GRAPHICS_API(IsVulkanLogicalDeviceValid(), "Reference to invalid Vulkan logical device inside {}", __FUNCTION__);

    for (VulkanShaderModuleGroup& group : m_shaderModuleGroups) {
        for (VulkanShaderModule& module : group.modules) {
            vkDestroyShaderModule(s_pLogicalDevice, module.pModule, nullptr);
        }
    }
}


VulkanShaderModule VulkanShaderSystem::CreateVulkanShaderModule(const VulkanShaderModuleIntermediateDataConfigInfo& configInfo) noexcept
{
    AM_ASSERT_GRAPHICS_API(configInfo.pGroupConfigInfo, "configInfo.pGroupConfigInfo is nullptr");
    AM_ASSERT_GRAPHICS_API(configInfo.pFilepath, "configInfo.pFilepath is nullptr");

    const fs::path& filepath = *configInfo.pFilepath;
    const std::string filename = filepath.filename().string();

    if (configInfo.kind >= VulkanShaderKind_COUNT) {
        AM_LOG_GRAPHICS_API_ERROR("Shader compilation error: '{}' invalid kind. Skiped", filename.c_str());
        return {};
    }
            
    std::optional<VulkanShaderIntermediateData> shaderIntermediateDataOpt = CreateVulkanShaderModuleIntermediateData(configInfo);
    if (!shaderIntermediateDataOpt.has_value()) {
        return {};
    }

    VulkanShaderIntermediateData& shaderIntermediateData = shaderIntermediateDataOpt.value();

    if (!GetSPIRVCode(shaderIntermediateData)) {
        AM_LOG_GRAPHICS_API_ERROR("Shader '{}' skiped", filename.c_str());
        return {};
    }

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderIntermediateData.spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode    = shaderIntermediateData.spirvCode.data();

    VkShaderModule pModule;
    if (vkCreateShaderModule(s_pLogicalDevice, &createInfo, nullptr, &pModule) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan shader module creation failed");
        return {};
    }

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Shader module '{}' created"), filename.c_str());

    VulkanShaderModule module = {};
    module.pModule = pModule;
    module.kind = configInfo.kind;

    return module;
}


void VulkanShaderSystem::AddVulkanShaderModuleGroup(const VulkanShaderModuleGroup& group) noexcept
{
    m_shaderModuleGroups.emplace_back(group);
}


bool VulkanShaderSystem::PreprocessShader(VulkanShaderIntermediateData &data) noexcept
{
    AM_ASSERT(IsInitialized(), "Calling {} while VulkanShaderSystem is not initialized", __FUNCTION__);

    std::vector<char>& sourceCode = data.code;

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Preprocessing '{}'..."), data.filename);

    shaderc::PreprocessedSourceCompilationResult result = m_compiler.PreprocessGlsl(sourceCode.data(), sourceCode.size(), 
        data.kind, data.filename.c_str(), data.compileOptions);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        AM_LOG_GRAPHICS_API_ERROR("Shader {} preprocess error:\n{}", data.filename, result.GetErrorMessage().c_str());
        return false;
    }

    const char* preprocessedSourceCode = result.cbegin();
    const size_t newSourceCodeSize = (uintptr_t)result.cend() - (uintptr_t)preprocessedSourceCode;
    
    sourceCode.resize(newSourceCodeSize);
    memcpy_s(sourceCode.data(), sourceCode.size(), preprocessedSourceCode, newSourceCodeSize);

#if defined(AM_LOGGING_ENABLED)
    std::string output(sourceCode.data(), sourceCode.size());
    AM_LOG_GRAPHICS_API_INFO("Preprocessed GLSL source code ({}):\n{}", data.filename, output.c_str());
#endif

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Preprocessing '{}' finished successfuly"), data.filename);

    return true;
}


bool VulkanShaderSystem::CompileToAssembly(VulkanShaderIntermediateData &data) noexcept
{
    AM_ASSERT(IsInitialized(), "Calling {} while VulkanShaderSystem is not initialized", __FUNCTION__);

    std::vector<char>& preproccessedSourceCode = data.code;

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Compling '{}' to assembly..."), data.filename);

    shaderc::AssemblyCompilationResult result = m_compiler.CompileGlslToSpvAssembly(preproccessedSourceCode.data(), preproccessedSourceCode.size(),
        data.kind, data.filename.c_str(), data.compileOptions);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        AM_LOG_GRAPHICS_API_ERROR("Shader {} compiling to assembly error:\n{}", data.filename, result.GetErrorMessage().c_str());
        return false;
    }

    const char* assemlyCodeRaw = result.cbegin();
    const size_t assemblyCodeSize = (uintptr_t)result.cend() - (uintptr_t)assemlyCodeRaw;
    
    std::vector<char>& assemblyCode = preproccessedSourceCode;

    assemblyCode.resize(assemblyCodeSize);
    memcpy_s(assemblyCode.data(), assemblyCode.size(), assemlyCodeRaw, assemblyCodeSize);

#if defined(AM_LOGGING_ENABLED)
    std::string output(assemblyCode.data(), assemblyCode.size());
    AM_LOG_GRAPHICS_API_INFO("SPIR-V assembly code ({}):\n{}", data.filename, output.c_str());
#endif

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Compling '{}' to assembly finished"), data.filename);

    return true;
}


bool VulkanShaderSystem::AssembleToSPIRV(VulkanShaderIntermediateData &data) noexcept
{
    AM_ASSERT(IsInitialized(), "Calling {} while VulkanShaderSystem is not initialized", __FUNCTION__);

    std::vector<char>& assemblyCode = data.code;

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Assembling '{}' to SPIR-V..."), data.filename);

    shaderc::SpvCompilationResult result = m_compiler.AssembleToSpv(assemblyCode.data(), assemblyCode.size(), data.compileOptions);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        AM_LOG_GRAPHICS_API_ERROR("Shader {} assembling to SPIR-V error:\n{}", data.filename, result.GetErrorMessage().c_str());
        return false;
    }

    std::vector<uint32_t>& spirvCode = data.spirvCode;

    const uint32_t* spirvCodeRaw = result.cbegin();
    const size_t spirvCodeSizeInU32 = result.cend() - result.cbegin();
    const size_t spirvCodeSizeInU8 = spirvCodeSizeInU32 * sizeof(uint32_t);

    spirvCode.resize(spirvCodeSizeInU32);
    memcpy_s(spirvCode.data(), spirvCodeSizeInU8, spirvCodeRaw, spirvCodeSizeInU8);

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Assembling '{}' to SPIR-V finished"), data.filename);

    return true;
}


bool VulkanShaderSystem::GetSPIRVCode(VulkanShaderIntermediateData & data) noexcept
{
    if (!PreprocessShader(data)) {
        return false;
    }

    if (!CompileToAssembly(data)) {
        return false;
    }

    if (!AssembleToSPIRV(data)) {
        return false;
    }

    return true;
}


void VulkanShaderSystem::CompileShaders() noexcept
{
    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Compiling shaders..."));

    m_shaderModuleGroups.clear();

    ForEachSubDirectory(paths::AM_PROJECT_SHADERS_DIR_PATH, [this](const fs::directory_entry& dirEntry)
    {
        std::optional<fs::path> configFilepathOpt = FindFirstFileInSubDirectoriesIf(dirEntry, [](const fs::directory_entry& dirEntry)
        {
            return IsShaderGroupConfigFile(dirEntry.path()); 
        });

        if (!configFilepathOpt.has_value()) {
            AM_LOG_GRAPHICS_API_ERROR("Can't find config JSON file in {} directory", dirEntry.path().string().c_str());
            return;
        }

        const std::optional<VulkanShaderGroupConfigInfo> groupConfigInfoOpt = GetVulkanShaderGroupConfigInfo(configFilepathOpt.value());

        if (!groupConfigInfoOpt.has_value()) {
            return;
        }

        const VulkanShaderGroupConfigInfo& groupConfigInfo = groupConfigInfoOpt.value();

        VulkanShaderModuleGroup moduleGroup;

        ForEachFileInSubDirectories(dirEntry.path(), [this, &moduleGroup, &groupConfigInfo](const fs::directory_entry& fileEntry)
        {
            const fs::path& filepath = fileEntry.path();

            if (IsShaderGroupConfigFile(filepath)) {
                return;
            }

            const fs::path extension = filepath.extension();

            const bool isVertexShader = IsVertexShaderFileCorrectExtension(extension);
            const bool isPixelShader = IsPixelShaderFileCorrectExtension(extension);

            if (!isVertexShader && !isPixelShader) {
                const fs::path filename = filepath.filename();
                AM_LOG_GRAPHICS_API_ERROR("Shader compilation error: '{}' has unrecognized extension '{}'. Skiped", filename.string().c_str(), extension.string().c_str());
                
                return;
            }

            VulkanShaderModuleIntermediateDataConfigInfo moduleConfig = {};
            moduleConfig.pGroupConfigInfo = &groupConfigInfo;
            moduleConfig.pFilepath = &filepath;
            moduleConfig.kind = isVertexShader ? VulkanShaderKind_VERTEX : isPixelShader ? VulkanShaderKind_PIXEL : VulkanShaderKind_COUNT;

            VulkanShaderModule shaderModule = CreateVulkanShaderModule(moduleConfig);
            if (shaderModule.IsVaild()) {
                moduleGroup.modules[shaderModule.kind] = shaderModule;
            }
        });

        AddVulkanShaderModuleGroup(moduleGroup);
    });

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Shaders compilation finished"));
}
