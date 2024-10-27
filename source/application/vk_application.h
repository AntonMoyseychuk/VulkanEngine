#pragma once

#include <string>
#include <cstdint>

#include "core.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#if defined(AM_OS_WINDOWS)
  #define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3native.h>


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
    VkInstance pInstance;

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
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
    VkDevice pDevice;

    std::vector<VkQueue> queues;
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
    VulkanSwapChainDesc      desc;
    std::vector<VkImage>     images;
    std::vector<VkImageView> imageViews;
    VkSwapchainKHR           pSwapChain;
};


struct VulkanRenderPass
{
    VkRenderPass pRenderPass;
};


struct VulkanGraphicsPipeline
{
    VkPipelineLayout pLayout;
    VkPipeline pPipeline;
};


struct VulkanFramebuffers
{
    bool IsValid() const noexcept
    {
        for (const VkFramebuffer& framebuffer : framebuffers) {
            if (framebuffer == VK_NULL_HANDLE) {
                return false;
            }
        }

        return true;
    }

    std::vector<VkFramebuffer> framebuffers;
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

    static bool InitVulkanDebugMessanger(const VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo) noexcept;
    static void TerminateVulkanDebugCallback() noexcept;

    static bool InitVulkanInstance() noexcept;
    static void TerminateVulkanInstance() noexcept;
    
    static bool InitVulkanSurface() noexcept;
    static void TerminateVulkanSurface() noexcept;

    static bool InitVulkanPhysicalDevice() noexcept;
    static void TerminateVulkanPhysicalDevice() noexcept;

    static bool InitVulkanLogicalDevice() noexcept;
    static void TerminateVulkanLogicalDevice() noexcept;

    static bool InitVulkanSwapChain() noexcept;
    static void TerminateVulkanSwapChain() noexcept;

    static bool InitVulkanRenderPass() noexcept;
    static void TerminateVulkanRenderPass() noexcept;

    static bool InitVulkanGraphicsPipeline() noexcept;
    static void TerminateVulkanGraphicsPipeline() noexcept;

    static bool InitVulkanFramebuffers() noexcept;
    static void TerminateVulkanFramebuffers() noexcept;

    static bool InitVulkan() noexcept;
    static void TerminateVulkan() noexcept;

    static bool IsGLFWWindowCreated() noexcept;

    static bool IsVulkanDebugMessangerInitialized() noexcept;
    static bool IsVulkanInstanceInitialized() noexcept;
    static bool IsVulkanSurfaceInitialized() noexcept;
    static bool IsVulkanPhysicalDeviceInitialized() noexcept;
    static bool IsVulkanLogicalDeviceInitialized() noexcept;
    static bool IsVulkanSwapChainInitialized() noexcept;
    static bool IsVulkanRenderPassInitialized() noexcept;
    static bool IsVulkanGraphicsPipelineInitialized() noexcept;
    static bool IsVulkanFramebuffersInitialized() noexcept;
    
    static bool IsVulkanInitialized() noexcept;

private:
    VulkanApplication(const VulkanAppInitInfo& appInitInfo);

    bool IsInstanceInitialized() const noexcept; 

private:
    static inline GLFWwindow* s_pGLFWWindow = nullptr;

    struct VulkanState
    {
        VulkanInstance          intance;
        VulkanPhysicalDevice    physicalDevice;
        VulkanLogicalDevice     logicalDevice;
        VulkanSurface           surface;
        VulkanSwapChain         swapChain;
        VulkanRenderPass        renderPass;
        VulkanGraphicsPipeline  graphicsPipeline;
        VulkanFramebuffers      framebuffers;
    };
    static inline std::unique_ptr<VulkanState> s_pVulkanState = nullptr;

    static inline std::unique_ptr<VulkanApplication> s_pAppInst = nullptr;

private:
    
};