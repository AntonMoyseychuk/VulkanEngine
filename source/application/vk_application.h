#pragma once

#include <string>
#include <cstdint>

#include <vulkan/vulkan.h>


struct GLFWwindow;


struct AppWindowInitInfo
{
    std::string title = {};

    uint32_t width = 0;
    uint32_t height = 0;
    
    bool resizable = false;
};


struct VulkanAppInitInfo
{
    AppWindowInitInfo windowInitInfo;
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
    std::vector<std::string> extensionNames;
};


struct VulkanQueueFamilies
{
    enum RequiredQueueFamilyType
    {
        RequiredQueueFamilyType_GRAPHICS,
        RequiredQueueFamilyType_PRESENT,
        RequiredQueueFamilyType_COUNT
    };

    struct QueueFamily
    {
        std::optional<uint32_t> index;
        float priority;
        VkQueueFlagBits flags;
    };

    VulkanQueueFamilies();
    bool IsComplete() const noexcept;

    std::vector<QueueFamily> families;
};


struct VulkanInstance
{
    ~VulkanInstance()
    {
        for (const char* extension : extensions) {
            delete[] extension;
        }

    #if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
        for (const char* layer : validationLayers) {
            delete[] layer;
        }
    #endif
    }

    VkInstance pInstance;

    std::vector<char*> extensions;

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    std::vector<char*> validationLayers;
#endif

#if defined(AM_LOGGING_ENABLED)
    VkDebugUtilsMessengerEXT pDebugMessenger;
#endif
};


struct VulkanPhysicalDevice
{
    VkPhysicalDevice            pDevice;

    VkPhysicalDeviceProperties  properties;
    VkPhysicalDeviceFeatures    features;

    VulkanQueueFamilies         queueFamilies;
};


struct VulkanLogicalDevice
{
    ~VulkanLogicalDevice()
    {
        for (const char* extension : extensions) {
            delete[] extension;
        }
    }

    VkDevice pDevice;

    std::vector<VkQueue> queues;
    std::vector<char*> extensions;
};


struct VulkanSurface
{
    VkSurfaceKHR pSurface;
};


struct VulkanSwapChainDesc
{
    bool IsValid() const noexcept { return !formats.empty() && !presentModes.empty(); }

    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;

    VkFormat                        currFormat;
    VkExtent2D                      currExtent;
};


struct VulkanSwapChain
{
    VulkanSwapChainDesc  desc;
    std::vector<VkImage> images;
    VkSwapchainKHR       pSwapChain;
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
    static bool CreateGLFWWindow(const AppWindowInitInfo& initInfo) noexcept;
    static void TerminateGLFWWindow() noexcept;

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    static bool InitVulkanDebugCallback(const VulkanDebugCallbackInitInfo& initInfo) noexcept;
    static void TerminateVulkanDebugCallback() noexcept;
#endif

    static bool InitVulkanInstance(const VulkanInstanceInitInfo& initInfo) noexcept;
    static void TerminateVulkanInstance() noexcept;
    
    static bool InitVulkanSurface() noexcept;
    static void TerminateVulkanSurface() noexcept;

    static bool InitVulkanPhysicalDevice(const VulkanPhysDeviceInitInfo& initInfo) noexcept;
    static void TerminateVulkanPhysicalDevice() noexcept;

    static bool InitVulkanLogicalDevice(const VulkanLogicalDeviceInitInfo& initInfo) noexcept;
    static void TerminateVulkanLogicalDevice() noexcept;

    static bool InitVulkanSwapChain() noexcept;
    static void TerminateVulkanSwapChain() noexcept;

    static bool InitVulkan() noexcept;
    static void TerminateVulkan() noexcept;

    static bool IsGLFWWindowCreated() noexcept;

    static bool IsVulkanInstanceInitialized() noexcept;
    static bool IsVulkanSurfaceInitialized() noexcept;
    static bool IsVulkanDebugCallbackInitialized() noexcept;
    static bool IsVulkanPhysicalDeviceInitialized() noexcept;
    static bool IsVulkanLogicalDeviceInitialized() noexcept;
    static bool IsVulkanSwapChainInitialized() noexcept;
    
    static bool IsVulkanInitialized() noexcept;

private:
    VulkanApplication(const VulkanAppInitInfo& appInitInfo);

    bool IsInstanceInitialized() const noexcept; 

private:
    static inline GLFWwindow* s_pGLFWWindow = nullptr;

    struct VulkanState
    {
        VulkanInstance       intance;
        VulkanPhysicalDevice physicalDevice;
        VulkanLogicalDevice  logicalDevice;
        VulkanSurface        surface;
        VulkanSwapChain      swapChain;
    };
    static inline std::unique_ptr<VulkanState> s_pVulkanState = nullptr;

    static inline std::unique_ptr<VulkanApplication> s_pAppInst = nullptr;

private:
    
};