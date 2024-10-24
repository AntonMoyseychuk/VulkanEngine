#pragma once

#include "shader_cache.h"

#include "utils/debug/assertion.h"
#include "utils/file/file.h"

#include <vulkan/vulkan.h>

#include <filesystem>
#include <memory>


// enum VulkanShaderKind : uint32_t
// {
//     VulkanShaderKind_VERTEX,
//     VulkanShaderKind_PIXEL,
//     VulkanShaderKind_COUNT,
// };


// struct VulkanShaderGroupConfigInfo
// {
//     std::vector<std::string> defines;
// };


// struct VulkanShaderModuleIntermediateDataConfigInfo
// {
//     const VulkanShaderGroupConfigInfo* pGroupConfigInfo;
//     const fs::path* pFilepath;
//     VulkanShaderKind kind;
// };


// struct VulkanShaderModule
// {
//     bool IsVaild() const noexcept { return kind < VulkanShaderKind_COUNT && pModule != VK_NULL_HANDLE; }

//     VkShaderModule pModule;
//     VulkanShaderKind kind;
// };


// struct VulkanShaderModuleGroup
// {
//     bool AreModulesVaild() const noexcept 
//     {
//         for (const VulkanShaderModule& module : modules) {
//             if (!module.IsVaild()) {
//                 return false;
//             }
//         }

//         return true;
//     }

//     bool IsModuleValid(VulkanShaderKind kind) const noexcept 
//     {
//         AM_ASSERT(kind < VulkanShaderKind_COUNT, "Invalid module kind");
//         return modules[kind].IsVaild();
//     }

//     std::array<VulkanShaderModule, VulkanShaderKind_COUNT> modules;
// };


// struct VulkanShaderIntermediateData;


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

    // High level method which either loads shaders from the shader cache, or compiles them, or both
    bool InitializeShaders() noexcept;

    // Force shaders recompiling and submiting to shader cache
    void RecompileShaders() noexcept;

private:
    static bool IsInstanceInitialized() noexcept;
    static bool IsVulkanLogicalDeviceValid() noexcept;

private:
    VulkanShaderSystem();

    bool IsShaderCacheInitialized() const noexcept;

    void ClearVulkanShaderModules() noexcept;

    void CompileShaders(bool forceRecompile = false) noexcept;

    // Creates shader module from file
    // Writes compiled code to shader cache
    bool BuildAndAddShaderModule(const void* pSetup, const ShaderID& shaderId) noexcept;

    // Creates shader module from shader cache
    bool LoadAndAddShaderModule(const ShaderID& shaderId) noexcept;

private:
    static inline std::unique_ptr<VulkanShaderSystem> s_pShaderSysInstace = nullptr;
    static inline VkDevice s_pLogicalDevice = VK_NULL_HANDLE;

private:
    std::unordered_map<ShaderIDProxy, VkShaderModule> m_shaderModules;

    std::unique_ptr<VulkanShaderCache> m_pShaderCache = nullptr;
};