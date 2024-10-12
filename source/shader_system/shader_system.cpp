#include "pch.h"
#include "shader_system.h"

#include "config.h"
#include "utils/debug/assertion.h"
#include "utils/file/file.h"


static const fs::path AM_VERTEX_SHADER_EXTENSION = ".vs";
static const fs::path AM_PIXEL_SHADER_EXTENSION = ".ps";
static const fs::path AM_SHADER_CONFIG_EXTENSION = ".json";


struct VulkanShaderConfigInfo
{
    
};


struct VulkanShaderIntermediateData
{
    std::vector<char>       code;
    std::vector<uint32_t>   spirvCode;

    shaderc::CompileOptions compileOptions;
    shaderc_shader_kind     kind;
            
    std::string             filename;
};


static VulkanShaderConfigInfo GetVulkanShaderConfigInfo(const fs::path& pathToConfigJson) noexcept
{
    VulkanShaderConfigInfo info = {};

    return info;
}


static VulkanShaderIntermediateData CreateVulkanShaderIntermediateData(const VulkanShaderConfigInfo& configInfo) noexcept
{
    VulkanShaderIntermediateData data = {};
    // data.compileOptions.

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
    if (IsVulkanLogicalDeviceValid()) {
        for (VulkanShaderModule& module : m_shaderModules) {
            vkDestroyShaderModule(s_pLogicalDevice, module.pModule, nullptr);
        }
    }
}


VulkanShaderModule VulkanShaderSystem::CreateVulkanShaderModule(const fs::directory_entry &shaderFileEntry) noexcept
{
    const fs::path& filepath = shaderFileEntry.path();

    const std::string filename = filepath.filename().string();
    const std::string extension = filepath.extension().string();
            
    const bool isVertexShader     = extension == AM_VERTEX_SHADER_EXTENSION;
    const bool isPixelShader      = extension == AM_PIXEL_SHADER_EXTENSION;
    const bool isShaderConfigFile = extension == AM_SHADER_CONFIG_EXTENSION;

    if (isShaderConfigFile) {
        return {};
    }
            
    const bool isCorrectShaderKind = isVertexShader || isPixelShader;
            
    if (!isCorrectShaderKind) {
        AM_LOG_GRAPHICS_API_ERROR("Shader compilation error: '{}' has unrecognized extension '{}'. Skiped", filename.c_str(), extension.c_str());
        return {};
    }

    std::optional<std::vector<char>> sourceCodeOpt = ReadFile(filepath);
            
    if(!sourceCodeOpt.has_value()) {
        AM_LOG_GRAPHICS_API_ERROR("Shader compilation error: '{}' invalid content. Skiped", filename.c_str());
        return {};
    }
            
    VulkanShaderIntermediateData data = {};
    data.filename   = filename.c_str();
    data.kind       = isVertexShader ? shaderc_vertex_shader : shaderc_fragment_shader;
    data.code       = std::move(sourceCodeOpt.value());

    if (!GetSPIRVCode(data)) {
        AM_LOG_GRAPHICS_API_ERROR("Shader '{}' skiped", filename.c_str());
        return {};
    }

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = data.spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode    = data.spirvCode.data();

    VkShaderModule pModule;
    if (vkCreateShaderModule(s_pLogicalDevice, &createInfo, nullptr, &pModule) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan shader module creation failed");
        return {};
    }

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Shader module '{}' created"), data.filename);

    VulkanShaderModule module = {};
    module.pModule = pModule;
    module.kind = isVertexShader ? VulkanShaderKind_VERTEX : VulkanShaderKind_PIXEL;

    return module;
}


void VulkanShaderSystem::AddVulkanShaderModule(const VulkanShaderModule& shaderModule) noexcept
{
    if (shaderModule.pModule == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("Passed VK_NULL_HANDLE pModule to {}. Skiped", __FUNCTION__);
        return;
    }

    m_shaderModules.emplace_back(shaderModule);
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

    ForEachFileInDirectory(paths::AM_PROJECT_SHADERS_DIR_PATH, [this](const fs::directory_entry& shaderFileEntry)
    {
        VulkanShaderModule module = CreateVulkanShaderModule(shaderFileEntry);
        AddVulkanShaderModule(module);
    });

    AM_LOG_GRAPHICS_API_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Shaders compilation finished"));
}
