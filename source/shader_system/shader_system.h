#pragma once

#include "shader_cache.h"

#include "utils/debug/assertion.h"
#include "utils/file/file.h"

#include <vulkan/vulkan.h>

#include <filesystem>
#include <memory>


class VulkanShaderGroupSetup;


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
    bool BuildAndAddShaderModule(const VulkanShaderGroupSetup* pSetup, const ShaderID& shaderId) noexcept;

    // Creates shader module from shader cache
    bool LoadAndAddShaderModule(const ShaderID& shaderId) noexcept;

private:
    static inline std::unique_ptr<VulkanShaderSystem> s_pShaderSysInstance = nullptr;
    static inline VkDevice s_pLogicalDevice = VK_NULL_HANDLE;

private:
    std::unordered_map<ShaderIDProxy, VkShaderModule> m_shaderModules;

    std::unique_ptr<VulkanShaderCache> m_pShaderCache = nullptr;
};