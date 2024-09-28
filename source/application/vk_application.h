#pragma once

#include <string>
#include <cstdint>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


struct VulkanAppInitInfo
{
    std::string title;

    uint32_t width;
    uint32_t height;

    bool resizable = false;
};


#if defined(AM_LOGGING_ENABLED)
struct VulkanDebugCallbackInitInfo
{
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity;
    VkDebugUtilsMessageTypeFlagsEXT messageType;
    PFN_vkDebugUtilsMessengerCallbackEXT pCallback;
};
#endif


class VulkanApplication
{
public:
    static VulkanApplication& Instance() noexcept;
    
    static bool Init() noexcept;
    static void Terminate() noexcept;

    VulkanApplication(const VulkanApplication& app) = delete;
    VulkanApplication& operator=(const VulkanApplication& app) = delete;
    
    VulkanApplication(VulkanApplication&& app) = delete;
    VulkanApplication& operator=(VulkanApplication&& app) = delete;

    ~VulkanApplication();

    void Run() noexcept;
    
private:
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    static bool InitVulkanDebugCallback(const VulkanDebugCallbackInitInfo& initInfo) noexcept;
    static void TerminateVulkanDebugCallback() noexcept;
#endif

    static bool InitVulkan(const char* appName) noexcept;
    static void TerminateVulkan() noexcept;

private:
    VulkanApplication(const VulkanAppInitInfo& appInitInfo);

    bool IsGlfwWindowCreated() const noexcept;

private:
    static inline std::unique_ptr<VulkanApplication> s_pAppInst = nullptr;
    static inline VkInstance s_vulkanInst = {};

#if defined(AM_LOGGING_ENABLED)
    static inline VkDebugUtilsMessengerEXT s_vulkanDebugMessenger = {};
#endif
    
    static inline bool s_isAppInitialized = false;

private:
    GLFWwindow* m_glfwWindow = nullptr;
};