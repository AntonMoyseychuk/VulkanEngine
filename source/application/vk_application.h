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
struct VulkanDebugCallbackInitInfo;
#endif


class VulkanApplication
{
public:
    static VulkanApplication& Instance() noexcept;
    
    static bool Init() noexcept;
    static void Terminate() noexcept;

    static bool IsInitialized() noexcept;

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

    bool IsInstanceInitialized() const noexcept;
    bool IsGlfwWindowCreated() const noexcept;

private:
    struct VulkanState
    {
        VkInstance instance;

        #if defined(AM_LOGGING_ENABLED)
            VkDebugUtilsMessengerEXT debugMessenger;
        #endif
    };

    static inline std::unique_ptr<VulkanApplication> s_pAppInst = nullptr;
    static inline VulkanState s_vulkanState = {};

private:
    GLFWwindow* m_glfwWindow = nullptr;
};