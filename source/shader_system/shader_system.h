#pragma once

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include <filesystem>
#include <memory>


enum VulkanShaderKind
{
    VulkanShaderKind_INVALID,
    VulkanShaderKind_VERTEX,
    VulkanShaderKind_PIXEL,
    VulkanShaderKind_COUNT,
};


struct VulkanShaderModule
{
    VkShaderModule pModule;
    VulkanShaderKind kind;
};


struct VulkanShaderIntermediateData;


class VulkanShaderSystem
{
public:
    static VulkanShaderSystem& Instance() noexcept;
    
    static bool Init(VkDevice pLogicalDevice) noexcept;
    static void Terminate() noexcept;

    static bool IsInitialized() noexcept;

    VulkanShaderSystem(const VulkanShaderSystem& sys) = delete;
    VulkanShaderSystem& operator=(const VulkanShaderSystem& sys) = delete;
    
    VulkanShaderSystem(VulkanShaderSystem&& sys) = delete;
    VulkanShaderSystem& operator=(VulkanShaderSystem&& sys) = delete;

    ~VulkanShaderSystem();

    VulkanShaderModule CreateVulkanShaderModule(const std::filesystem::directory_entry& shaderFileEntry) noexcept;
    void AddVulkanShaderModule(const VulkanShaderModule& shaderModule) noexcept;

    void RecompileShaders() noexcept;

private:
    static bool IsInstanceInitialized() noexcept;
    static bool IsVulkanLogicalDeviceValid() noexcept;

private:
    VulkanShaderSystem();

    bool PreprocessShader(VulkanShaderIntermediateData& data) noexcept;
    bool CompileToAssembly(VulkanShaderIntermediateData& data) noexcept;
    bool AssembleToSPIRV(VulkanShaderIntermediateData& data) noexcept;

    bool GetSPIRVCode(VulkanShaderIntermediateData& data) noexcept;

    void CompileShaders() noexcept;

private:
    static inline std::unique_ptr<VulkanShaderSystem> s_pShaderSysInstace = nullptr;
    static inline VkDevice s_pLogicalDevice = VK_NULL_HANDLE;

private:
    shaderc::Compiler m_compiler;

    std::vector<VulkanShaderModule> m_shaderModules;
};