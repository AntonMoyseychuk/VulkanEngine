#include "pch.h"
#include "vk_application.h"

#include "path_system/path_system.h"

#include "utils/debug/assertion.h"
#include "utils/json/json.h"
#include "utils/file/file.h"

#include "shader_system/shader_system.h"


static constexpr const char* ENGINE_NAME = "Engine";
static constexpr const char* APPLICATION_NAME   = "Application";

// Application config JSON field names
static constexpr const char* JSON_APP_CONFIG_WINDOW_FIELD_NAME = "window";
static constexpr const char* JSON_APP_CONFIG_WINDOW_TITLE_FIELD_NAME = "title";
static constexpr const char* JSON_APP_CONFIG_WINDOW_WIDTH_FIELD_NAME = "width";
static constexpr const char* JSON_APP_CONFIG_WINDOW_HEIGHT_FIELD_NAME = "height";
static constexpr const char* JSON_APP_CONFIG_WINDOW_RESIZABLE_FLAG_FIELD_NAME = "resizable";


static constexpr std::array<float, VulkanQueueFamilies::RequiredQueueFamilyType_COUNT> FAMILY_PRIORITIES = { 
    1.0f,
    1.0f,
};
    
static constexpr std::array<VkQueueFlagBits, VulkanQueueFamilies::RequiredQueueFamilyType_COUNT> REQUIRED_QUIE_FAMILIES_FLAGS = {
    VK_QUEUE_GRAPHICS_BIT,
    VkQueueFlagBits{}       // Present family doesn't have flag bits
};


static std::optional<VulkanAppInitInfo> ParseAppInitInfoJson(const fs::path& pathToJson) noexcept
{
    using namespace amjson;

    const std::optional<nlohmann::json> appInitInfoJsonOpt = amjson::ParseJson(pathToJson);
    if (!appInitInfoJsonOpt.has_value()) {
        return {};
    }
    
    const nlohmann::json& appInitInfoJson = appInitInfoJsonOpt.value();

    const nlohmann::json& windowConfigJson = GetJsonSubNode(appInitInfoJson, JSON_APP_CONFIG_WINDOW_FIELD_NAME);

    VulkanAppInitInfo appInitInfo = {};
    GetJsonSubNode(windowConfigJson, JSON_APP_CONFIG_WINDOW_TITLE_FIELD_NAME).get_to(appInitInfo.windowInitInfo.title);
    GetJsonSubNode(windowConfigJson, JSON_APP_CONFIG_WINDOW_WIDTH_FIELD_NAME).get_to(appInitInfo.windowInitInfo.width);
    GetJsonSubNode(windowConfigJson, JSON_APP_CONFIG_WINDOW_HEIGHT_FIELD_NAME).get_to(appInitInfo.windowInitInfo.height);
    GetJsonSubNode(windowConfigJson, JSON_APP_CONFIG_WINDOW_RESIZABLE_FLAG_FIELD_NAME).get_to(appInitInfo.windowInitInfo.resizable);

    return appInitInfo;
}


#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}


static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{   
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        AM_LOG_GRAPHICS_API_INFO(pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        AM_LOG_GRAPHICS_API_INFO(pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        AM_LOG_GRAPHICS_API_WARN(pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        AM_ASSERT_GRAPHICS_API(false, pCallbackData->pMessage);
    } else {
        const std::string message = fmt::format("[UNKNOWN]: {}", pCallbackData->pMessage);
        AM_LOG_GRAPHICS_API_INFO(message.c_str());
    }
    
    return VK_FALSE;
}


static VkDebugUtilsMessengerCreateInfoEXT CreateVkDebugUtilsMessengerCreateInfo() noexcept
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.pfnUserCallback = VulkanDebugCallback;
    createInfo.pUserData = nullptr;
    
    createInfo.messageSeverity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    
    createInfo.messageType = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    return createInfo;
}
#endif


static inline std::string GetVulkanFlagsDesc(const VkSurfaceTransformFlagBitsKHR& transformBits) noexcept
{
    std::string result = "";

    if (transformBits & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (transformBits & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (transformBits & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (transformBits & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (transformBits & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (transformBits & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (transformBits & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (transformBits & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (transformBits & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (transformBits & VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR) {
        result += AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR" AM_OUTPUT_COLOR_RESET_ASCII_CODE " | ";
    }

    if (!result.empty()) {
        result.resize(result.size() - sizeof(" | ") + 1);
    }

    return result;
}


AM_MAYBE_UNUSED static inline std::string GetVulkanObjectDesc(const VkLayerProperties& layer) noexcept
{
    return fmt::format("{} -> {}{}{}", layer.layerName, AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, layer.description, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
}

AM_MAYBE_UNUSED static inline std::string GetVulkanObjectDesc(const VkExtensionProperties& extension) noexcept
{
    return fmt::format("{}{}{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, extension.extensionName, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
}

AM_MAYBE_UNUSED static inline std::string GetVulkanObjectDesc(const VkExtent2D& extent) noexcept
{
    return fmt::format("{}[{}x{}]{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, extent.width, extent.height, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
}

AM_MAYBE_UNUSED static std::string GetVulkanObjectDesc(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) noexcept
{
    const std::string descStr = fmt::format(
        "    - Current Extent:            {};\n" 
        "    - Current Transform:         {};\n"
        "    - Max Image Array layers:    " AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "{}" AM_OUTPUT_COLOR_RESET_ASCII_CODE ";\n"
        "    - Max Image Count:           " AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "{}" AM_OUTPUT_COLOR_RESET_ASCII_CODE ";\n"
        "    - Max Image Extent:          {};\n"
        "    - Min Image Count:           " AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "{}" AM_OUTPUT_COLOR_RESET_ASCII_CODE ";\n"
        "    - Min Image Extent:          {};\n"
        "    - Supported Composite Alpha: " AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "{}" AM_OUTPUT_COLOR_RESET_ASCII_CODE ";\n"
        "    - Supported Transforms:      " AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "{}" AM_OUTPUT_COLOR_RESET_ASCII_CODE ";\n"
        "    - Supported Usage Flags:     " AM_OUTPUT_COLOR_YELLOW_ASCII_CODE "{}" AM_OUTPUT_COLOR_RESET_ASCII_CODE ";\n",
            GetVulkanObjectDesc(surfaceCapabilities.currentExtent),
            GetVulkanFlagsDesc(surfaceCapabilities.currentTransform),
            surfaceCapabilities.maxImageArrayLayers,
            surfaceCapabilities.maxImageCount,
            GetVulkanObjectDesc(surfaceCapabilities.maxImageExtent),
            surfaceCapabilities.minImageCount,
            GetVulkanObjectDesc(surfaceCapabilities.minImageExtent),
            surfaceCapabilities.supportedCompositeAlpha,
            surfaceCapabilities.supportedTransforms,
            surfaceCapabilities.supportedUsageFlags
        );
        
    return descStr;
}


AM_MAYBE_UNUSED static inline std::string GetVulkanObjectDesc(const char* str) noexcept
{
    AM_ASSERT_GRAPHICS_API(str != nullptr, "str is nullptr");
    return fmt::format("{}{}{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, str, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
}


AM_MAYBE_UNUSED static inline std::string GetVulkanObjectDesc(const std::string& str) noexcept
{
    return GetVulkanObjectDesc(str.c_str());
}


template<typename VkObjT>
static std::string MakeVulkanObjectsListString(const VkObjT* pObjects, size_t objectsCount) noexcept
{
    if (objectsCount == 0) {
        return "";
    }

    AM_ASSERT_GRAPHICS_API(pObjects != nullptr, "pObjects is nullptr");

    std::string result = "";
        
    for (size_t i = 0; i < objectsCount; ++i) {
        result += fmt::format("    - {};\n", GetVulkanObjectDesc(pObjects[i]));
    }

    return result;
}


template<typename VkObjT>
static std::string MakeVulkanObjectsListString(const std::vector<VkObjT>& objects) noexcept
{
    return MakeVulkanObjectsListString(objects.data(), objects.size());
}


AM_MAYBE_UNUSED static std::vector<std::string> GetVulkanInstanceValidationLayers(const nlohmann::json& vkInstanceBuildJson, const char* pNodeName) noexcept
{
    std::vector<std::string> requestedLayers = amjson::ParseJsonSubNodeToArray<std::string>(vkInstanceBuildJson, pNodeName);

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    AM_LOG_GRAPHICS_API_INFO("Available Vulkan validation layers:\n{}", MakeVulkanObjectsListString(availableLayers));
    
    std::vector<std::string> result;
    result.reserve(requestedLayers.size());

    for (const std::string& requestedLayer : requestedLayers) {
        const auto FindPred = [&requestedLayer](const VkLayerProperties& prop) {
            return requestedLayer == prop.layerName;
        };

        if (std::find_if(availableLayers.cbegin(), availableLayers.cend(), FindPred) != availableLayers.cend()) {
            result.emplace_back(requestedLayer);
        } else {
            AM_LOG_GRAPHICS_API_ERROR("Requested {} Vulkan validation layer in not found", requestedLayer);
        }
    }

    result.shrink_to_fit();
    AM_LOG_GRAPHICS_API_INFO("Included Vulkan validation layers:\n{}", MakeVulkanObjectsListString(result));

    return result;
}


static std::vector<std::string> GetVulkanInstanceExtensions(const nlohmann::json& vkInstanceBuildJson, const char* pNodeName) noexcept
{
    std::vector<std::string> requestedExtensions = amjson::ParseJsonSubNodeToArray<std::string>(vkInstanceBuildJson, pNodeName);

    uint32_t availableVulkanExtCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableVulkanExtCount, nullptr);

    std::vector<VkExtensionProperties> availableVkExtensions(availableVulkanExtCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableVulkanExtCount, availableVkExtensions.data());

    AM_LOG_GRAPHICS_API_INFO("Available Vulkan instance extensions:\n{}", MakeVulkanObjectsListString(availableVkExtensions));
    
    std::vector<std::string> result;
    result.reserve(requestedExtensions.size());

    for (const std::string& requestedExtension : requestedExtensions) {
        const auto FindPred = [&requestedExtension](const VkExtensionProperties& prop) {
            return requestedExtension == prop.extensionName;
        };

        if (std::find_if(availableVkExtensions.cbegin(), availableVkExtensions.cend(), FindPred) != availableVkExtensions.cend()) {
            result.emplace_back(requestedExtension);
        } else {
            AM_LOG_GRAPHICS_API_WARN("Requested {} Vulkan extension in not found. Skipped", requestedExtension);
        }
    }

    result.shrink_to_fit();
    AM_LOG_GRAPHICS_API_INFO("Included Vulkan instance extensions:\n{}", MakeVulkanObjectsListString(result));

    return result;
}


static bool CheckVulkanLogicalDeviceExtensionSupport(const VulkanPhysicalDevice& physicalDevice, const char* const* requiredExtensions, size_t requiredExtensionCount) noexcept
{
    AM_ASSERT_GRAPHICS_API(physicalDevice.pDevice != VK_NULL_HANDLE, "Invalid Vulkan physical device handle");
    
    if (requiredExtensionCount == 0) {
        return true;
    }

    AM_ASSERT_GRAPHICS_API(requiredExtensions, "requiredExtensions is nullptr");

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice.pDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice.pDevice, nullptr, &extensionCount, availableExtensions.data());
    
    AM_LOG_GRAPHICS_API_INFO("Available Vulkan logical device extensions:\n{}", MakeVulkanObjectsListString(availableExtensions));

    for (size_t i = 0; i < requiredExtensionCount; ++i) {
        const char* pExtension = requiredExtensions[i];
        AM_ASSERT_GRAPHICS_API(pExtension, "Extension is nullptr");

        bool isLogicalDeviceExtensionSuppoted = false;
        
        for (const VkExtensionProperties& availableExtension : availableExtensions) {
            if (strcmp(pExtension, availableExtension.extensionName) == 0) {
                isLogicalDeviceExtensionSuppoted = true;
                break;
            }
        }

        if (!isLogicalDeviceExtensionSuppoted) {
            AM_ASSERT_GRAPHICS_API(false, "Vulkan logical device doesn't support required extensions '{}'", pExtension);
            return false;
        }
    }

    return true;
}


static inline constexpr const char* ConvertVkPhysicalDeviceTypeToStr(VkPhysicalDeviceType type) noexcept
{
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return "discrete GPU";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return "integrated GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return "CPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return "virtual GPU";
            break;
        default:
            return nullptr;
    }
}


static uint64_t GetVulkanDeviceTypePriority(VkPhysicalDeviceType type) noexcept
{
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: 
            return 4;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return 3;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return 2;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return 1;
        default:
            AM_ASSERT_GRAPHICS_API(false, "Unsupported Vulkan physical device type");
            return 0;
    }
}


static void PrintPhysicalDeviceProperties(const VkPhysicalDeviceProperties& props) noexcept
{
#if defined(AM_LOGGING_ENABLED)
    const uint32_t deviceID = props.deviceID;
    const char* deviceName = props.deviceName;

    const char* typeStr = ConvertVkPhysicalDeviceTypeToStr(props.deviceType);
    const char* deviceType = typeStr ? typeStr : "unknown";

    const uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(props.apiVersion);
    const uint32_t apiVersionMinor = VK_API_VERSION_MINOR(props.apiVersion);
    const uint32_t apiVersionPatch = VK_API_VERSION_PATCH(props.apiVersion);
    
    const uint32_t driverVersionMajor = VK_API_VERSION_MAJOR(props.driverVersion);
    const uint32_t driverVersionMinor = VK_API_VERSION_MINOR(props.driverVersion);
    const uint32_t driverVersionPatch = VK_API_VERSION_PATCH(props.driverVersion);

    constexpr const char* format = "Device {} properties:\n" 
        "    - Name:           " AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "{}") ";\n"
        "    - Type:           " AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "{}") ";\n"
        "    - API Version:    " AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "{}.{}.{}") ";\n"
        "    - Driver Version: " AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "{}.{}.{}") ".";

    AM_LOG_GRAPHICS_API_INFO(format, deviceID, deviceName, deviceType, apiVersionMajor, apiVersionMinor, apiVersionPatch, driverVersionMajor, driverVersionMinor, driverVersionPatch);
#endif
}


static void PrintPhysicalDeviceFeatures(const VkPhysicalDeviceFeatures& features) noexcept
{
#if defined(AM_LOGGING_ENABLED)
    // TODO: implement    
#endif
}


static VulkanQueueFamilies FindRequiredVulkanQueueFamilies(VkPhysicalDevice pPhysicalDevice, VkSurfaceKHR pSurface) noexcept
{
    if (pPhysicalDevice == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE pPhysicalDevice passed to {}", __FUNCTION__);
        return {};
    }

    VulkanQueueFamilies result = {};

    uint32_t familiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevice, &familiesCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(familiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevice, &familiesCount, queueFamilies.data());

    for (uint32_t requiredFamilyIndex = 0; requiredFamilyIndex < result.families.size(); ++requiredFamilyIndex) {
        for (uint32_t familyIndex = 0; familyIndex < queueFamilies.size(); ++familyIndex) {
            if (requiredFamilyIndex == VulkanQueueFamilies::RequiredQueueFamilyType_PRESENT) {
                VkBool32 isPresentSupported = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(pPhysicalDevice, familyIndex, pSurface, &isPresentSupported);

                if (isPresentSupported) {
                    result.families[requiredFamilyIndex].index = familyIndex;
                }
            }
            
            const VkQueueFlags& queueFlags = queueFamilies[familyIndex].queueFlags;

            if (queueFlags & result.families[requiredFamilyIndex].flags) {
                result.families[requiredFamilyIndex].index = familyIndex;
                break;
            }
        }
    }

    return result;
}


static VulkanPhysicalDevice GetVulkanPhysicalDeviceInternal(VkPhysicalDevice pPhysicalDevice, VkSurfaceKHR pSurface) noexcept
{
    if (pPhysicalDevice == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE pPhysicalDevice passed to {}", __FUNCTION__);
        return {};
    }

    VulkanPhysicalDevice device = {};
    device.pDevice = pPhysicalDevice;

    vkGetPhysicalDeviceProperties(pPhysicalDevice, &device.properties);
    PrintPhysicalDeviceProperties(device.properties);
    
    vkGetPhysicalDeviceFeatures(pPhysicalDevice, &device.features);
    PrintPhysicalDeviceFeatures(device.features);

    device.queueFamilies = FindRequiredVulkanQueueFamilies(pPhysicalDevice, pSurface);

    return device;
}


static uint64_t GetVulkanPhysicalDevicePriority(const VulkanPhysicalDevice& device) noexcept
{
    if (device.pDevice == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE device.pDevice passed to {}", __FUNCTION__);
        return 0;
    }

    uint64_t priority = GetVulkanDeviceTypePriority(device.properties.deviceType);

    AM_MAYBE_UNUSED const char* message = priority > 0 ? 
        AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Device '{}' with type '{}' match required types\n") :
        AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_RED_ASCII_CODE, "Device '{}' with type '{}' doesn't matches required types\n");
    AM_LOG_GRAPHICS_API_INFO(message, device.properties.deviceName, ConvertVkPhysicalDeviceTypeToStr(device.properties.deviceType));
    
    return priority;
}


static VulkanSwapChainDesc GetVulkanSwapChainDeviceSurfaceDesc(const VulkanPhysicalDevice& device, const VulkanSurface& surface) noexcept
{
    if (device.pDevice == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE device.pDevice passed to {}", __FUNCTION__);
        return {};
    }

    if (surface.pSurface == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE surface.pSurface passed to {}", __FUNCTION__);
        return {};
    }

    VulkanSwapChainDesc desc = {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.pDevice, surface.pSurface, &desc.capabilities);
    AM_LOG_GRAPHICS_API_INFO("Vulkan physical device surface capabilities:\n{}", GetVulkanObjectDesc(desc.capabilities));

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.pDevice, surface.pSurface, &formatCount, nullptr);

    if (formatCount == 0) {
        AM_ASSERT_GRAPHICS_API(false, "There are no any available physical device surface formats");
        return {};
    }

    desc.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.pDevice, surface.pSurface, &formatCount, desc.formats.data());

    uint32_t presentModesCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.pDevice, surface.pSurface, &presentModesCount, nullptr);

    if (presentModesCount == 0) {
        AM_ASSERT_GRAPHICS_API(false, "There are no any available physical device surface formats");
        return {};
    }

    desc.presentModes.resize(presentModesCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.pDevice, surface.pSurface, &presentModesCount, desc.presentModes.data());

    return desc;
}


const VkSurfaceFormatKHR& PickSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) noexcept
{
    // Create configuration file node for this in future
    for (const VkSurfaceFormatKHR& format : availableFormats) {
        if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return availableFormats[0];
}


VkPresentModeKHR PickSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) noexcept
{
    // Create configuration file node for this in future
    for (const VkPresentModeKHR& presentMode : availablePresentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D PickSwapChainSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t framebufferWidth, uint32_t framebufferHeight) noexcept
{
    if (capabilities.currentExtent.width != std::numeric_limits<decltype(capabilities.currentExtent.width)>::max()) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {};
    actualExtent.width = std::clamp(framebufferWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(framebufferWidth, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    
    return actualExtent; 
}


VulkanQueueFamilies::VulkanQueueFamilies()
    : families(RequiredQueueFamilyType_COUNT)
{
    for (size_t i = 0; i < RequiredQueueFamilyType_COUNT; ++i) {
        families[i].priority = FAMILY_PRIORITIES[i];
        families[i].flags    = REQUIRED_QUIE_FAMILIES_FLAGS[i];
    }
}


bool VulkanQueueFamilies::IsComplete() const noexcept
{
    return std::find_if(families.cbegin(), families.cend(), [](const auto& desc) {
        return !desc.index.has_value();
    }) == families.cend();
}


VulkanApplication& VulkanApplication::Instance() noexcept
{
    AM_ASSERT(s_pAppInst != nullptr, "Application is not initialized, call Application::Init(...) first");
    
    return *s_pAppInst;
}


bool VulkanApplication::Init() noexcept
{
    if (IsInitialized()) {
        AM_LOG_WARN("Application is already initialized");
        return true;
    }

    amInitLogSystem();
    
    if (!PathSystem::Init()) {
        return false;
    }

    std::optional<VulkanAppInitInfo> appInitInfoOpt = ParseAppInitInfoJson(PathSystem::GetProjectConfigFilepath());
    if (!appInitInfoOpt.has_value()) {
        return false;
    }

    const VulkanAppInitInfo& appInitInfo = appInitInfoOpt.value();

    if (!CreateGLFWWindow(appInitInfo.windowInitInfo)) {
        return false;
    }

    if (!InitVulkan()) {
        Terminate();
        return false;
    }

    s_pAppInst = std::unique_ptr<VulkanApplication>(new VulkanApplication(appInitInfo));
    if (!IsInitialized()) {
        AM_ASSERT(false, "VulkanApplication instance initialization failed");
        Terminate();
        return false;
    }

    glfwShowWindow(s_pGLFWWindow);

    return true;
}


void VulkanApplication::Terminate() noexcept
{
    s_pAppInst = nullptr;

    TerminateVulkan();
    TerminateGLFWWindow();

    amTerminateLogSystem();
}


bool VulkanApplication::IsInitialized() noexcept
{
    return amIsLogSystemInitialized() && PathSystem::IsInitialized() && s_pAppInst && s_pAppInst->IsInstanceInitialized();
}


void VulkanApplication::Run() noexcept
{
    while(!glfwWindowShouldClose(s_pGLFWWindow)) {
        glfwPollEvents();
    }
}


bool VulkanApplication::CreateGLFWWindow(const AppWindowInitInfo &initInfo) noexcept
{
    if (IsGLFWWindowCreated()) {
        AM_LOG_WARN("GLFW window is already created");
        return true;
    }

#if defined(AM_LOGGING_ENABLED)
    glfwSetErrorCallback([](int errorCode, const char* description) -> void {
        AM_LOG_WINDOW_ERROR("{} (code: {})", description, errorCode);
    });
#endif
    
    if (glfwInit() != GLFW_TRUE) {
        AM_ASSERT_WINDOW(false, "GLFW initialization failed");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, initInfo.resizable ? GLFW_TRUE : GLFW_FALSE);

    s_pGLFWWindow = glfwCreateWindow((int)initInfo.width, (int)initInfo.height, initInfo.title.c_str(), nullptr, nullptr);
    const bool isWindowCreated = s_pGLFWWindow != nullptr;

    glfwHideWindow(s_pGLFWWindow);
    
    AM_ASSERT_WINDOW(isWindowCreated, "GLFW window creation failed");
    
    return isWindowCreated;
}


void VulkanApplication::TerminateGLFWWindow() noexcept
{
    glfwDestroyWindow(s_pGLFWWindow);
    glfwTerminate();
}


bool VulkanApplication::InitVulkanDebugMessanger(const VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo) noexcept
{
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    if (IsVulkanDebugMessangerInitialized()) {
        AM_LOG_WARN("Vulkan debug messanger is already initialized");
        return true;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan debug message callback..."));

    if (CreateDebugUtilsMessengerEXT(s_pVulkanState->intance.pInstance, &messengerCreateInfo, nullptr, &s_pVulkanState->intance.pDebugMessenger) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan debug callback initialization failed");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan debug message callback initialization finished..."));
#endif
    
    return true;
}


void VulkanApplication::TerminateVulkanDebugCallback() noexcept
{
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    if (s_pVulkanState) {
        DestroyDebugUtilsMessengerEXT(s_pVulkanState->intance.pInstance, s_pVulkanState->intance.pDebugMessenger, nullptr);
    }
#endif
}


bool VulkanApplication::InitVulkanInstance() noexcept
{
    if (IsVulkanInstanceInitialized()) {
        AM_LOG_WARN("Vulkan instance is already initialized");
        return true;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan instance..."));

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = APPLICATION_NAME;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = ENGINE_NAME;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instCreateInfo = {};
    instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.pApplicationInfo = &appInfo;
    instCreateInfo.enabledLayerCount = 0;
    instCreateInfo.ppEnabledLayerNames = nullptr;
    instCreateInfo.ppEnabledExtensionNames = nullptr;

    static constexpr const char* vulkanInstanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
    
    #if defined(AM_OS_WINDOWS)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    #endif

    #if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    #endif
    };

    instCreateInfo.ppEnabledExtensionNames = vulkanInstanceExtensions;
    instCreateInfo.enabledExtensionCount = _countof(vulkanInstanceExtensions);
    
    VkDebugUtilsMessengerCreateInfoEXT vulkanDebugMessangerCreateInfo = {};

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    static constexpr const char* vulkanInstanceValidationLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    instCreateInfo.ppEnabledLayerNames = vulkanInstanceValidationLayers;
    instCreateInfo.enabledLayerCount = _countof(vulkanInstanceValidationLayers);
    
    vulkanDebugMessangerCreateInfo = CreateVkDebugUtilsMessengerCreateInfo();
    
    instCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&vulkanDebugMessangerCreateInfo;
#endif

    if (vkCreateInstance(&instCreateInfo, nullptr, &s_pVulkanState->intance.pInstance) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan instance creation failed");
        return false;
    }

    if (!InitVulkanDebugMessanger(vulkanDebugMessangerCreateInfo)) {
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan instance initialization finished"));
    return true;
}


void VulkanApplication::TerminateVulkanInstance() noexcept
{
    TerminateVulkanDebugCallback();

    if (s_pVulkanState) {
        vkDestroyInstance(s_pVulkanState->intance.pInstance, nullptr);
    }
}


bool VulkanApplication::InitVulkanSurface() noexcept
{
    if (IsVulkanSurfaceInitialized()) {
        AM_LOG_WARN("Vulkan surface is already initialized");
        return true;
    }

#if defined(AM_OS_WINDOWS)
    if (!IsGLFWWindowCreated()) {
        AM_ASSERT(false, "GLFW window is not created. Create it before {}", __FUNCTION__);
        return false;
    }

    if (!IsVulkanInstanceInitialized()) {
        AM_ASSERT(false, "Vulkan Instance is not created. Create it before {}", __FUNCTION__);
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan surface..."));

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = glfwGetWin32Window(s_pGLFWWindow);
    surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

    if (vkCreateWin32SurfaceKHR(s_pVulkanState->intance.pInstance, &surfaceCreateInfo, nullptr, &s_pVulkanState->surface.pSurface) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan surface creation failed");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan surface initialization finished"));
    return true;
#else
    return false;
#endif
}


void VulkanApplication::TerminateVulkanSurface() noexcept
{
    if (s_pVulkanState) {
        vkDestroySurfaceKHR(s_pVulkanState->intance.pInstance, s_pVulkanState->surface.pSurface, nullptr);
    }
}


bool VulkanApplication::InitVulkanPhysicalDevice() noexcept
{
    if (IsVulkanPhysicalDeviceInitialized()) {
        AM_LOG_WARN("Vulkan physical device is already initialized");
        return true;
    }

    if (!IsVulkanInstanceInitialized()) {
        AM_ASSERT(false, "Vulkan instance must be initialized before physical device initialization");
        return false;
    }

    if (!IsVulkanSurfaceInitialized()) {
        AM_ASSERT(false, "Vulkan surface must be initialized before physical device initialization");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan physical device..."));

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(s_pVulkanState->intance.pInstance, &deviceCount, nullptr);

    if (deviceCount <= 0) {
        AM_ASSERT_GRAPHICS_API(false, "There are no any physical graphics devices that support Vulkan");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(s_pVulkanState->intance.pInstance, &deviceCount, devices.data());

    AM_LOG_GRAPHICS_API_INFO("Picking suitable physical device...");

    struct SuitablePhysicalDevice
    {
        VulkanPhysicalDevice physicalDevice;
        VulkanSwapChainDesc  swapChainDesc;
    };

    std::multimap<uint64_t, SuitablePhysicalDevice> suitableDevices;
    
    for (VkPhysicalDevice pPhysicalDevice : devices) {
        SuitablePhysicalDevice device = {};

        VulkanPhysicalDevice& physDevice = device.physicalDevice;
        VulkanSwapChainDesc& swapChainDesc = device.swapChainDesc;

        physDevice = GetVulkanPhysicalDeviceInternal(pPhysicalDevice, s_pVulkanState->surface.pSurface);

        if (!physDevice.queueFamilies.IsComplete()) {
            AM_LOG_GRAPHICS_API_WARN("Physical device '{}' doesn't have required queue families. Skiped.", physDevice.properties.deviceName);
            continue;
        }

        swapChainDesc = GetVulkanSwapChainDeviceSurfaceDesc(physDevice, s_pVulkanState->surface);
        
        if (!swapChainDesc.IsValid()) {
            AM_LOG_GRAPHICS_API_WARN("Physical device '{}' doesn't support swap chain with required properties. Skiped.", physDevice.properties.deviceName);
            continue;
        }

        const uint64_t devicePriority = GetVulkanPhysicalDevicePriority(physDevice);

        if (devicePriority == 0) {
            AM_LOG_GRAPHICS_API_WARN("Physical device '{}' has 0 priority. Skiped.", physDevice.properties.deviceName);
            continue;
        }

        suitableDevices.insert(std::make_pair(devicePriority, device));
    }

    if (suitableDevices.empty()) {
        AM_ASSERT_GRAPHICS_API(false, "There are no any Vulkan physical graphics devices that support required properties");
        return false;
    }

    s_pVulkanState->physicalDevice = std::move(suitableDevices.rbegin()->second.physicalDevice);
    s_pVulkanState->swapChain.desc = std::move(suitableDevices.rbegin()->second.swapChainDesc);

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan physical device initialization finished"));
    return true;
}


void VulkanApplication::TerminateVulkanPhysicalDevice() noexcept
{
}


bool VulkanApplication::InitVulkanLogicalDevice() noexcept
{
    if (IsVulkanLogicalDeviceInitialized()) {
        AM_LOG_WARN("Vulkan logical device is already initialized");
        return true;
    }

    if (!IsVulkanPhysicalDeviceInitialized()) {
        AM_ASSERT(false, "Vulkan physical device must be initialized before logical device initialization");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan logical device..."));

    const VulkanPhysicalDevice& physicalDevice                          = s_pVulkanState->physicalDevice;
    const std::vector<VulkanQueueFamilies::QueueFamily>& queueFamilies  = s_pVulkanState->physicalDevice.queueFamilies.families;

    static constexpr const char* vulkanLogicalDeviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    static constexpr size_t vulkanLogicalDeviceExtensionCount = _countof(vulkanLogicalDeviceExtensions);

    if (!CheckVulkanLogicalDeviceExtensionSupport(physicalDevice, vulkanLogicalDeviceExtensions, vulkanLogicalDeviceExtensionCount)) {
        return false;
    }

    AM_LOG_GRAPHICS_API_INFO("Included Vulkan logical device extensions:\n{}", MakeVulkanObjectsListString(vulkanLogicalDeviceExtensions, vulkanLogicalDeviceExtensionCount));

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pEnabledFeatures = &physicalDevice.features;
    
// #if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
//     createInfo.enabledLayerCount = s_pVulkanState->intance.validationLayers.size();
//     createInfo.ppEnabledLayerNames = createInfo.enabledLayerCount ? s_pVulkanState->intance.validationLayers.data() : nullptr;
// #endif

    createInfo.ppEnabledExtensionNames = vulkanLogicalDeviceExtensions;
    createInfo.enabledExtensionCount = vulkanLogicalDeviceExtensionCount;
    
    std::vector<VkDeviceQueueCreateInfo> deviceQueueInfos(queueFamilies.size());

    for (size_t i = 0; i < queueFamilies.size(); ++i) {
        deviceQueueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueInfos[i].queueFamilyIndex = queueFamilies[i].index.value();
        deviceQueueInfos[i].queueCount = 1;
        deviceQueueInfos[i].pQueuePriorities = &queueFamilies[i].priority;
    }

    createInfo.pQueueCreateInfos = deviceQueueInfos.data();
    createInfo.queueCreateInfoCount = deviceQueueInfos.size();

    if (vkCreateDevice(s_pVulkanState->physicalDevice.pDevice, &createInfo, nullptr, &s_pVulkanState->logicalDevice.pDevice) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan logical device creation failed");
        return false;
    }

    VkDevice pDevice = s_pVulkanState->logicalDevice.pDevice;
    
    s_pVulkanState->logicalDevice.queues.resize(VulkanQueueFamilies::RequiredQueueFamilyType_COUNT);
    for (uint32_t i = 0; i < VulkanQueueFamilies::RequiredQueueFamilyType_COUNT; ++i) {
        const uint32_t queueFamilyIndex = s_pVulkanState->physicalDevice.queueFamilies.families[i].index.value();
        VkQueue* ppQueue = &s_pVulkanState->logicalDevice.queues[i];

        vkGetDeviceQueue(pDevice, queueFamilyIndex, 0, ppQueue);

        if (*ppQueue == VK_NULL_HANDLE) {
            AM_ASSERT_GRAPHICS_API(false, "Failed to get device queue");
            return false;
        }
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan logical device initialization finished"));
    return true;
}


void VulkanApplication::TerminateVulkanLogicalDevice() noexcept
{
    if (s_pVulkanState) {
        vkDestroyDevice(s_pVulkanState->logicalDevice.pDevice, nullptr);
    }
}


bool VulkanApplication::InitVulkanSwapChain() noexcept
{
    if (IsVulkanSwapChainInitialized()) {
        AM_LOG_WARN("Vulkan swap chain is already initialized");
        return true;
    }

    if (!IsGLFWWindowCreated()) {
        AM_ASSERT(false, "Window must be initialized before Vulkan swap chain initialization");
        return false;
    }

    if (!IsVulkanPhysicalDeviceInitialized()) {
        AM_ASSERT(false, "Vulkan phisical device must be initialized before swap chain initialization");
        return false;
    }

    if (!IsVulkanLogicalDeviceInitialized()) {
        AM_ASSERT(false, "Vulkan logical device must be initialized before swap chain initialization");
        return false;
    }

    if (!IsVulkanSurfaceInitialized()) {
        AM_ASSERT(false, "Vulkan surface must be initialized before swap chain initialization");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan swap chain..."));

    VulkanSwapChainDesc& swapChainDesc = s_pVulkanState->swapChain.desc;

    if (!swapChainDesc.IsValid()) {
        AM_ASSERT(false, "Vulkan swap chain description is invalid");
        return false;
    }

    const VkPresentModeKHR presentMode = PickSwapChainPresentMode(swapChainDesc.presentModes);
    const VkSurfaceFormatKHR& surfaceFormat = PickSwapChainSurfaceFormat(swapChainDesc.formats);

    int framebufferWidth = 0, framebufferHeight = 0;
    glfwGetFramebufferSize(s_pGLFWWindow, &framebufferWidth, &framebufferHeight);
    const VkExtent2D extent = PickSwapChainSurfaceExtent(swapChainDesc.capabilities, (uint32_t)framebufferWidth, (uint32_t)framebufferHeight);

    uint32_t imageCount = swapChainDesc.capabilities.minImageCount + 1;

    if (swapChainDesc.capabilities.maxImageCount > 0 && imageCount > swapChainDesc.capabilities.maxImageCount) {
        imageCount = swapChainDesc.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = s_pVulkanState->surface.pSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const std::vector<VulkanQueueFamilies::QueueFamily>& queueFamilies = s_pVulkanState->physicalDevice.queueFamilies.families;
    
    std::vector<uint32_t> familyIndices(queueFamilies.size());
    for (size_t i = 0; i < familyIndices.size(); ++i) {
        familyIndices[i] = queueFamilies[i].index.value();
    }

    if (familyIndices[VulkanQueueFamilies::RequiredQueueFamilyType_GRAPHICS] != familyIndices[VulkanQueueFamilies::RequiredQueueFamilyType_PRESENT]) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = familyIndices.data();
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainDesc.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(s_pVulkanState->logicalDevice.pDevice, &createInfo, nullptr, &s_pVulkanState->swapChain.pSwapChain) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan swap chain creation failed");
        return false;
    }

    uint32_t finalImageCount = 0;
    vkGetSwapchainImagesKHR(s_pVulkanState->logicalDevice.pDevice, s_pVulkanState->swapChain.pSwapChain, &finalImageCount, nullptr);
    
    std::vector<VkImage>& swapChainImages = s_pVulkanState->swapChain.images;
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(s_pVulkanState->logicalDevice.pDevice, s_pVulkanState->swapChain.pSwapChain, &imageCount, swapChainImages.data());

    swapChainDesc.currExtent = extent;
    swapChainDesc.currFormat = surfaceFormat.format;

    std::vector<VkImageView>& swapChainImageViews = s_pVulkanState->swapChain.imageViews;
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image        = swapChainImages[i];
        createInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format       = s_pVulkanState->swapChain.desc.currFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel    = 0;
        createInfo.subresourceRange.levelCount      = 1;
        createInfo.subresourceRange.baseArrayLayer  = 0;
        createInfo.subresourceRange.layerCount      = 1;

        if (vkCreateImageView(s_pVulkanState->logicalDevice.pDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            AM_ASSERT_GRAPHICS_API(false, "Vulkan image view {} creation failed", i);
            return false;
        }
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan swap chain initialization finished"));

    return true;
}


void VulkanApplication::TerminateVulkanSwapChain() noexcept
{
    if (s_pVulkanState) {
        for (VkImageView& imageView : s_pVulkanState->swapChain.imageViews) {
            vkDestroyImageView(s_pVulkanState->logicalDevice.pDevice, imageView, nullptr);
        }

        vkDestroySwapchainKHR(s_pVulkanState->logicalDevice.pDevice, s_pVulkanState->swapChain.pSwapChain, nullptr);
    }
}


bool VulkanApplication::InitVulkanRenderPass() noexcept
{
    if (IsVulkanRenderPassInitialized()) {
        AM_LOG_WARN("Vulkan render pass is already initialized");
        return true;
    }

    if (!IsVulkanLogicalDeviceInitialized()) {
        AM_ASSERT(false, "Vulkan logical device must be initialized before render pass initialization");
        return false;
    }

    if (!IsVulkanSwapChainInitialized()) {
        AM_ASSERT(false, "Vulkan logical device must be initialized before render pass initialization");
        return false;
    }

    VkAttachmentDescription colorAttachmentInfo = {};
    colorAttachmentInfo.format = s_pVulkanState->swapChain.desc.currFormat;
    colorAttachmentInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentInfo.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentInfo.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentInfoRef = {};
    colorAttachmentInfoRef.attachment = 0;
    colorAttachmentInfoRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassInfo = {};
    subpassInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassInfo.colorAttachmentCount = 1;
    subpassInfo.pColorAttachments = &colorAttachmentInfoRef;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachmentInfo;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassInfo;

    VkDevice pLogicalDevice = s_pVulkanState->logicalDevice.pDevice;
    VkRenderPass& pRenderPass = s_pVulkanState->renderPass.pRenderPass;

    if (vkCreateRenderPass(pLogicalDevice, &renderPassCreateInfo, nullptr, &pRenderPass) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan render pass creation failed");
        return false;
    }

    return true;
}


void VulkanApplication::TerminateVulkanRenderPass() noexcept
{
    if (s_pVulkanState) {
        vkDestroyRenderPass(s_pVulkanState->logicalDevice.pDevice, s_pVulkanState->renderPass.pRenderPass, nullptr);
    }
}


bool VulkanApplication::InitVulkanGraphicsPipeline() noexcept
{
    if (IsVulkanGraphicsPipelineInitialized()) {
        AM_LOG_WARN("Vulkan graphics pipeline is already initialized");
        return true;
    }

    if (!VulkanShaderSystem::IsInitialized()) {
        AM_ASSERT(false, "VulkanShaderSystem must be initialized before graphics pipeline initialization");
        return false;
    }

    if (!IsVulkanLogicalDeviceInitialized()) {
        AM_ASSERT(false, "Vulkan logical device must be initialized before graphics pipeline initialization");
        return false;
    }

    if (!IsVulkanSwapChainInitialized()) {
        AM_ASSERT(false, "Vulkan swap chain must be initialized before graphics pipeline initialization");
        return false;
    }

    if (!IsVulkanRenderPassInitialized()) {
        AM_ASSERT(false, "Vulkan render pass must be initialized before graphics pipeline initialization");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan graphics pipeline..."));

    VulkanShaderSystem& shaderSystem = VulkanShaderSystem::Instance();
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

    // Temp solution
    ShaderIDProxy vsIdProxy = ShaderID((PathSystem::GetProjectShadersSourceCodeDirectory() / "base\\base.vs").string(), {});
    vertShaderStageInfo.module = shaderSystem.m_shaderModules[vsIdProxy];
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo pixShaderStageInfo = {};
    pixShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pixShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Temp solution
    ShaderIDProxy psIdProxy = ShaderID((PathSystem::GetProjectShadersSourceCodeDirectory() / "base\\base.fs").string(), {});
    pixShaderStageInfo.module = shaderSystem.m_shaderModules[psIdProxy];
    pixShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, pixShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewportInfo = {};
    viewportInfo.x = 0.0f;
    viewportInfo.y = 0.0f;
    viewportInfo.width = s_pVulkanState->swapChain.desc.currExtent.width;
    viewportInfo.height = s_pVulkanState->swapChain.desc.currExtent.height;
    viewportInfo.minDepth = 0.0f;
    viewportInfo.maxDepth = 1.0f;

    VkRect2D scissorInfo = {};
    scissorInfo.offset = { 0, 0 };
    scissorInfo.extent = s_pVulkanState->swapChain.desc.currExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.pViewports = &viewportInfo;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pScissors = &scissorInfo;
    viewportStateCreateInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizerCreateInfo.depthBiasClamp = 0.0f;
    rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingCreateInfo.minSampleShading = 1.0f;
    multisamplingCreateInfo.pSampleMask = nullptr;
    multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; 
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
    colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendingCreateInfo.attachmentCount = 1;
    colorBlendingCreateInfo.pAttachments = &colorBlendAttachmentState;
    colorBlendingCreateInfo.blendConstants[0] = 0.0f;
    colorBlendingCreateInfo.blendConstants[1] = 0.0f;
    colorBlendingCreateInfo.blendConstants[2] = 0.0f;
    colorBlendingCreateInfo.blendConstants[3] = 0.0f;

    VkDynamicState dynamicStates[] = { 
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;
    dynamicStateCreateInfo.dynamicStateCount = _countof(dynamicStates);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkDevice pLogicalDevice = s_pVulkanState->logicalDevice.pDevice;
    VkPipelineLayout& pPipelineLayout = s_pVulkanState->graphicsPipeline.pLayout;

    if (vkCreatePipelineLayout(pLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &pPipelineLayout) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan pipeline layout creation failed");
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pPipelineLayout;
    pipelineCreateInfo.renderPass = s_pVulkanState->renderPass.pRenderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    VkPipeline& pPipeline = s_pVulkanState->graphicsPipeline.pPipeline;

    if (vkCreateGraphicsPipelines(pLogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pPipeline) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan pipeline creation failed");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan graphics pipeline initialization finished"));

    return true;
}


void VulkanApplication::TerminateVulkanGraphicsPipeline() noexcept
{
    if (s_pVulkanState) {
        VkDevice pLogicalDevice = s_pVulkanState->logicalDevice.pDevice;

        vkDestroyPipelineLayout(pLogicalDevice, s_pVulkanState->graphicsPipeline.pLayout, nullptr);
        vkDestroyPipeline(pLogicalDevice, s_pVulkanState->graphicsPipeline.pPipeline, nullptr);
    }
}


bool VulkanApplication::InitVulkanFramebuffers() noexcept
{
    if (IsVulkanFramebuffersInitialized()) {
        AM_LOG_WARN("Vulkan framebuffers are already initialized");
        return true;
    }

    if (!IsVulkanLogicalDeviceInitialized()) {
        AM_ASSERT(false, "Vulkan logical device must be initialized before framebuffers initialization");
        return false;
    }

    if (!IsVulkanSwapChainInitialized()) {
        AM_ASSERT(false, "Vulkan swap chain must be initialized before framebuffers initialization");
        return false;
    }

    if (!IsVulkanRenderPassInitialized()) {
        AM_ASSERT(false, "Vulkan render pass must be initialized before framebuffers initialization");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan framebuffers..."));

    const VulkanSwapChain& swapChain = s_pVulkanState->swapChain;
    const std::vector<VkImageView>& swapChainImageViews = swapChain.imageViews;
    const size_t swapChainImageViewsCount = swapChainImageViews.size();

    AM_ASSERT_GRAPHICS_API(swapChainImageViewsCount > 0, "There is no any image views in swap chain");

    std::vector<VkFramebuffer>& framebuffers = s_pVulkanState->framebuffers.framebuffers;
    framebuffers.resize(swapChainImageViewsCount);

    for (size_t i = 0; i < swapChainImageViewsCount; ++i) {
        VkImageView imageViewAttachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = s_pVulkanState->renderPass.pRenderPass;
        framebufferCreateInfo.attachmentCount = _countof(imageViewAttachments);
        framebufferCreateInfo.pAttachments = imageViewAttachments;
        framebufferCreateInfo.width = swapChain.desc.currExtent.width;
        framebufferCreateInfo.height = swapChain.desc.currExtent.height;
        framebufferCreateInfo.layers = _countof(imageViewAttachments);

        if (vkCreateFramebuffer(s_pVulkanState->logicalDevice.pDevice, &framebufferCreateInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            AM_ASSERT_GRAPHICS_API(false, "Vulkan framebuffers creation failed");
            return false;
        }
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan framebuffers initialization finished"));

    return true;
}


void VulkanApplication::TerminateVulkanFramebuffers() noexcept
{
    if (s_pVulkanState) {
        for (VkFramebuffer& pFramebuffer : s_pVulkanState->framebuffers.framebuffers) {
            vkDestroyFramebuffer(s_pVulkanState->logicalDevice.pDevice, pFramebuffer, nullptr);
        }
    }
}


bool VulkanApplication::InitVulkan() noexcept
{
    using namespace amjson;

    if (IsVulkanInitialized()) {
        AM_LOG_WARN("Vulkan is already initialized");
        return true;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan..."));

    s_pVulkanState = std::make_unique<VulkanState>();

    if (!InitVulkanInstance()) {
        return false;
    }

    if (!InitVulkanSurface()) {
        return false;
    }
    
    if (!InitVulkanPhysicalDevice()) {
        return false;
    }

    if (!InitVulkanLogicalDevice()) {
        return false;
    }

    if (!VulkanShaderSystem::Init(s_pVulkanState->logicalDevice.pDevice)) {
        return false;
    }

    if (!InitVulkanSwapChain()) {
        return false;
    }

    if (!InitVulkanRenderPass()) {
        return false;
    }

    if (!InitVulkanGraphicsPipeline()) {
        return false;
    }

    if (!InitVulkanFramebuffers()) {
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan initialization finished"));

    return true;
}


void VulkanApplication::TerminateVulkan() noexcept
{
    TerminateVulkanFramebuffers();
    TerminateVulkanGraphicsPipeline();
    TerminateVulkanRenderPass();
    TerminateVulkanSwapChain();
    VulkanShaderSystem::Terminate();
    TerminateVulkanLogicalDevice();
    TerminateVulkanPhysicalDevice();
    TerminateVulkanSurface();
    TerminateVulkanInstance();

    s_pVulkanState = nullptr;
}


bool VulkanApplication::IsGLFWWindowCreated() noexcept
{
    return s_pGLFWWindow != nullptr;
}


bool VulkanApplication::IsVulkanDebugMessangerInitialized() noexcept
{
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    return s_pVulkanState && s_pVulkanState->intance.pDebugMessenger != VK_NULL_HANDLE;
#else
    return true;
#endif
}

bool VulkanApplication::IsVulkanInstanceInitialized() noexcept
{
    return s_pVulkanState && s_pVulkanState->intance.pInstance != VK_NULL_HANDLE && IsVulkanDebugMessangerInitialized();
}


bool VulkanApplication::IsVulkanSurfaceInitialized() noexcept
{
    return s_pVulkanState && s_pVulkanState->surface.pSurface != VK_NULL_HANDLE;
}


bool VulkanApplication::IsVulkanPhysicalDeviceInitialized() noexcept
{
    return s_pVulkanState && 
        s_pVulkanState->physicalDevice.pDevice != VK_NULL_HANDLE && 
        s_pVulkanState->physicalDevice.queueFamilies.IsComplete();
}


bool VulkanApplication::IsVulkanLogicalDeviceInitialized() noexcept
{
    return s_pVulkanState && s_pVulkanState->logicalDevice.pDevice != VK_NULL_HANDLE;
}


bool VulkanApplication::IsVulkanSwapChainInitialized() noexcept
{
    if (!s_pVulkanState) {
        return false;
    }

    const VulkanSwapChain& swapChain = s_pVulkanState->swapChain;

    if (swapChain.pSwapChain == VK_NULL_HANDLE) {
        return false;
    }

    if (swapChain.imageViews.size() != swapChain.images.size() || swapChain.imageViews.empty()) {
        return false;
    }

    for (const VkImageView& imageView : swapChain.imageViews) {
        if (imageView == VK_NULL_HANDLE) {
            return false;
        }
    }

    return true;
}


bool VulkanApplication::IsVulkanRenderPassInitialized() noexcept
{
    return s_pVulkanState && s_pVulkanState->renderPass.pRenderPass != VK_NULL_HANDLE;
}


bool VulkanApplication::IsVulkanGraphicsPipelineInitialized() noexcept
{
    return s_pVulkanState 
        && s_pVulkanState->graphicsPipeline.pLayout != VK_NULL_HANDLE
        && s_pVulkanState->graphicsPipeline.pPipeline != VK_NULL_HANDLE;
}


bool VulkanApplication::IsVulkanFramebuffersInitialized() noexcept
{
    return s_pVulkanState && s_pVulkanState->framebuffers.IsValid();
}


bool VulkanApplication::IsVulkanInitialized() noexcept
{
    return IsVulkanInstanceInitialized()  
        && IsVulkanSurfaceInitialized()
        && IsVulkanPhysicalDeviceInitialized()
        && IsVulkanLogicalDeviceInitialized()
        && IsVulkanSwapChainInitialized()
        && IsVulkanRenderPassInitialized()
        && IsVulkanGraphicsPipelineInitialized();
}


VulkanApplication::VulkanApplication(const VulkanAppInitInfo &appInitInfo)
{
    
}


bool VulkanApplication::IsInstanceInitialized() const noexcept
{
    return true;
}


VulkanApplication::~VulkanApplication()
{
    
}