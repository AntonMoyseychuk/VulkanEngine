#pragma once

#include "shader_cache.h"

#include "utils/debug/assertion.h"
#include "utils/file/file.h"

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include <filesystem>
#include <memory>


enum VulkanShaderKind : uint32_t
{
    VulkanShaderKind_VERTEX,
    VulkanShaderKind_PIXEL,
    VulkanShaderKind_COUNT,
};


struct VulkanShaderGroupConfigInfo
{
    std::vector<std::string> defines;
};


struct VulkanShaderModuleIntermediateDataConfigInfo
{
    const VulkanShaderGroupConfigInfo* pGroupConfigInfo;
    const fs::path* pFilepath;
    VulkanShaderKind kind;
};


struct VulkanShaderModule
{
    bool IsVaild() const noexcept { return kind < VulkanShaderKind_COUNT && pModule != VK_NULL_HANDLE; }

    VkShaderModule pModule;
    VulkanShaderKind kind;
};


struct VulkanShaderModuleGroup
{
    bool AreModulesVaild() const noexcept 
    {
        for (const VulkanShaderModule& module : modules) {
            if (!module.IsVaild()) {
                return false;
            }
        }

        return true;
    }

    bool IsModuleValid(VulkanShaderKind kind) const noexcept 
    {
        AM_ASSERT(kind < VulkanShaderKind_COUNT, "Invalid module kind");
        return modules[kind].IsVaild();
    }

    std::array<VulkanShaderModule, VulkanShaderKind_COUNT> modules;
};


struct VulkanShaderIntermediateData;


class VulkanShaderSystem
{
    friend class VulkanApplication;
    
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

    void ClearShaderModuleGroups() noexcept;

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

    VulkanShaderModule CreateVulkanShaderModule(const VulkanShaderModuleIntermediateDataConfigInfo& configInfo) noexcept;

private:
    static inline std::unique_ptr<VulkanShaderSystem> s_pShaderSysInstace = nullptr;
    static inline VkDevice s_pLogicalDevice = VK_NULL_HANDLE;

private:
    shaderc::Compiler m_compiler;

    std::vector<VulkanShaderModuleGroup> m_shaderModuleGroups;

    std::unique_ptr<VulkanShaderCache> m_pShaderCache = nullptr;
};