#pragma once

#include <string>
#include <cstdint>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


struct VulkanAppInitInfo
{
    std::string title = {};

    uint32_t width = 0;
    uint32_t height = 0;
    
    bool resizable = false;
};


#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
struct VulkanDebugCallbackInitInfo
{
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity;
    VkDebugUtilsMessageTypeFlagsEXT messageType;
    PFN_vkDebugUtilsMessengerCallbackEXT pCallback;
};
#endif


struct VulkanInstanceInitInfo
{
    std::vector<std::string> extensionNames;
    std::vector<std::string> validationLayerNames;

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    VulkanDebugCallbackInitInfo debugCallbackInitInfo;
#endif
};


struct VulkanPhysDeviceInitInfo
{
    std::vector<std::string> types;
};


struct VulkanLogicalDeviceInitInfo
{

};


struct VulkanQueueFamilyDescs
{
    enum RequiredQueueFamilyType
    {
        RequiredQueueFamilyType_GRAPHICS,
        RequiredQueueFamilyType_COUNT
    };

    struct Desc
    {
        std::optional<uint32_t> index;
        float priority;
        VkQueueFlagBits flags;
    };

    VulkanQueueFamilyDescs();
    bool IsComplete() const noexcept;

    std::vector<Desc> descs;
};


struct VulkanPhysicalDeviceDesc
{
    VkPhysicalDevice pPhysicalDevice;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    VulkanQueueFamilyDescs queueFamilies;
};


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

    static bool InitVulkanInstance(const VulkanInstanceInitInfo& initInfo) noexcept;
    static void TerminateVulkanInstance() noexcept;

    static bool InitVulkanPhysicalDevice(const VulkanPhysDeviceInitInfo& initInfo) noexcept;
    static void TerminateVulkanPhysicalDevice() noexcept;

    static bool InitVulkanLogicalDevice(const VulkanLogicalDeviceInitInfo& initInfo) noexcept;
    static void TerminateVulkanLogicalDevice() noexcept;

    static bool InitVulkan() noexcept;
    static void TerminateVulkan() noexcept;


    static bool IsVulkanInstanceInitialized() noexcept;
    static bool IsVulkanDebugCallbackInitialized() noexcept;
    static bool IsVulkanPhysicalDeviceInitialized() noexcept;
    static bool IsVulkanLogicalDeviceInitialized() noexcept;
    
    static bool IsVulkanInitialized() noexcept;

private:
    VulkanApplication(const VulkanAppInitInfo& appInitInfo);

    bool IsInstanceInitialized() const noexcept;
    bool IsGlfwWindowCreated() const noexcept;

private:
    struct VulkanState
    {
        VkInstance pInstance;
        VulkanPhysicalDeviceDesc physicalDeviceDesc;
        VkDevice pLogicalDevice;

        #if defined(AM_LOGGING_ENABLED)
            VkDebugUtilsMessengerEXT debugMessenger;
        #endif
    };

    static inline std::unique_ptr<VulkanApplication> s_pAppInst = nullptr;
    static inline VulkanState s_vulkanState = {};

private:
    GLFWwindow* m_glfwWindow = nullptr;
};