#include "pch.h"
#include "vk_application.h"

#include "config.h"
#include "utils/debug/assertion.h"
#include "utils/json/json.h"
#include "utils/file/file.h"

#include "shader_system/shader_system.h"


static constexpr const char* VULKAN_ENGINE_NAME = "Vulkan Engine";
static constexpr const char* APPLICATION_NAME   = "Vulkan Application";

static constexpr const char* JSON_VK_CONFIG_OS_FIELD_NAME = "os";
static constexpr const char* JSON_VK_CONFIG_BUILD_TYPE_FIELD_NAME = "build_type";

#if defined(AM_OS_WINDOWS)
    static constexpr const char* JSON_VK_CONFIG_OS_NAME = "win32";
#endif

#if defined(AM_DEBUG)
    static constexpr const char* JSON_VK_CONFIG_BUILD_TYPE = "debug";
#else
    static constexpr const char* JSON_VK_CONFIG_BUILD_TYPE = "release";
#endif


// Vulkan instance config specific field names
static constexpr const char* JSON_VK_CONFIG_INSTANCE_FIELD_NAME = "instance";

static constexpr const char* JSON_VK_CONFIG_INSTANCE_VALIDATION_LAYERS_FIELD_NAME = "validation_layers";
static constexpr const char* JSON_VK_CONFIG_INSTANCE_EXTENSIONS_FIELD_NAME = "extensions";

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_FIELD_NAME = "callback";
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_FIELD_NAME = "severity";
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPES_FIELD_NAME = "message_types";

    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_ALL = "all";
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_ERROR = "error";
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_WARNING = "warning";
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_INFO = "info";
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_VERBOSE = "verbose";

    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_ALL = "all";
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_GENERAL = "general";
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_VALIDATION = "validation";
    static constexpr const char* JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_PERFORMANCE = "performance";
#endif


// Vulkan physical device config specific field names
static constexpr const char* JSON_VK_CONFIG_PHYS_DEVICE_FIELD_NAME = "physical_device";
static constexpr const char* JSON_VK_CONFIG_PHYS_DEVICE_TYPES_FIELD_NAME = "types";


// Vulkan logical device config specific field names
static constexpr const char* JSON_VK_CONFIG_LOGICAL_DEVICE_FIELD_NAME = "logical_device";
static constexpr const char* JSON_VK_CONFIG_LOGICAL_DEVICE_EXTENSIONS_FIELD_NAME = "extensions";


struct ParsedVulkanPhysicalDeviceType
{
    const char* str;
    const uint64_t priority;
};

static constexpr ParsedVulkanPhysicalDeviceType JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPE_VIRTUAL    = {"virtual", 1};
static constexpr ParsedVulkanPhysicalDeviceType JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPE_CPU        = {"cpu", 2};
static constexpr ParsedVulkanPhysicalDeviceType JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPE_INTEGRATED = {"integrated", 3};
static constexpr ParsedVulkanPhysicalDeviceType JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPE_DISCRETE   = {"discrete", 4};


// Application config specific field names
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


AM_MAYBE_UNUSED static const nlohmann::json& GetVkInstanceOsBuildTypeSpecificJsonObj(const nlohmann::json& base) noexcept
{
    const nlohmann::json& os = base[JSON_VK_CONFIG_OS_FIELD_NAME][JSON_VK_CONFIG_OS_NAME];
    return os[JSON_VK_CONFIG_BUILD_TYPE_FIELD_NAME][JSON_VK_CONFIG_BUILD_TYPE];
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


static VkDebugUtilsMessageSeverityFlagBitsEXT GetVkDebugUtilsMessageSeverityFlagBits(const std::vector<std::string>& tokens)
{
    constexpr uint32_t SEVERITY_ALL = (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    static const auto ConvertStrToSeverityBits = [SEVERITY_ALL](const std::string& severity) -> uint32_t
    {
        uint32_t severityBits = 0;

        if (severity == JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_ALL) {
            severityBits = SEVERITY_ALL;
        } else if (severity == JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_ERROR) {
            severityBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        } else if (severity == JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_WARNING) {
            severityBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        } else if (severity == JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_INFO) {
            severityBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        } else if (severity == JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_VERBOSE) {
            severityBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        } else {
            AM_ASSERT(false, "Invalid Vulkan callback message severity JSON tokken {}. Must be one of:\n    - {};\n    - {};\n    - {};\n    - {};\n    - {};", 
                severity, 
                JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_ALL, 
                JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_ERROR, 
                JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_WARNING,
                JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_INFO,
                JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_VERBOSE);
        }

        return severityBits;
    };

    uint32_t severity = 0;
    
    for (const std::string& token : tokens) {
        uint32_t temp = ConvertStrToSeverityBits(token);
        severity |= temp;

        if (temp == SEVERITY_ALL) {
            break;
        }
    }

    return (VkDebugUtilsMessageSeverityFlagBitsEXT)severity;
}


static VkDebugUtilsMessageTypeFlagsEXT GetVkDebugUtilsMessageTypeFlags(const std::vector<std::string>& tokens)
{
    constexpr uint32_t TYPE_ALL = (uint32_t)VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
        (uint32_t)VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
        (uint32_t)VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    static const auto ConvertStrToTypeBits = [TYPE_ALL](const std::string& type) -> uint32_t
    {
        uint32_t typeBits = 0;

        if (type == JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_ALL) {
            typeBits = TYPE_ALL;
        } else if (type == JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_GENERAL) {
            typeBits = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        } else if (type == JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_VALIDATION) {
            typeBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        } else if (type == JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_PERFORMANCE) {
            typeBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        } else {
            AM_ASSERT(false, "Invalid Vulkan callback message type JSON tokken {}. Must be one of:\n    - {};\n    - {};\n    - {};\n    - {};", 
                type, 
                JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_ALL, 
                JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_GENERAL, 
                JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_VALIDATION,
                JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPE_PERFORMANCE);
        }

        return typeBits;
    };

    uint32_t typeBits = 0;
    
    for (const std::string& token : tokens) {
        uint32_t temp = ConvertStrToTypeBits(token);
        typeBits |= temp;

        if (temp == TYPE_ALL) {
            break;
        }
    }

    return (VkDebugUtilsMessageTypeFlagsEXT)typeBits;
}


static VulkanDebugCallbackInitInfo ParseVulkanCallbackInitInfoJson(const nlohmann::json& vkInstanceCallbackJson) noexcept
{
    VulkanDebugCallbackInitInfo result = {};
    result.pCallback = VulkanDebugCallback;

    const nlohmann::json& severity = vkInstanceCallbackJson[JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_FIELD_NAME];
    std::vector<std::string> severityTokens = amjson::ParseJsonArray<std::string>(severity);
    result.messageSeverity = GetVkDebugUtilsMessageSeverityFlagBits(severityTokens);

    const nlohmann::json& messageTypes = vkInstanceCallbackJson[JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPES_FIELD_NAME];
    std::vector<std::string> messageTypeTokens = amjson::ParseJsonArray<std::string>(messageTypes);
    result.messageType = GetVkDebugUtilsMessageTypeFlags(messageTypeTokens);

    return result;
}


static VkDebugUtilsMessengerCreateInfoEXT GetVkDebugUtilsMessengerCreateInfo(const VulkanDebugCallbackInitInfo& initInfo) noexcept
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = initInfo.messageSeverity;
    createInfo.messageType = initInfo.messageType;
    createInfo.pfnUserCallback = initInfo.pCallback;
    createInfo.pUserData = nullptr;

    return createInfo;
}
#endif


static void CopyStringArrayToCharPtrArray(char** dest, const std::string* src, size_t size) noexcept
{
    AM_ASSERT(dest, "dest is nullptr");
    AM_ASSERT(src,  "src is nullptr");

    for(size_t i = 0; i < size; ++i) {
        const size_t copyCharsCount = src[i].size() + 1;

        dest[i] = new char[copyCharsCount];
        memset(dest[i], '\0', copyCharsCount);
        strcpy_s(dest[i], copyCharsCount, src[i].data());
    }
}


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

AM_MAYBE_UNUSED static inline std::string GetVulkanObjectDesc(const std::string& str) noexcept
{
    return fmt::format("{}{}{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, str, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
}

template<typename VkObjT>
static std::string MakeVulkanObjectsListString(const std::vector<VkObjT>& objects) noexcept
{
    std::string result = "";
        
    for (const VkObjT& object : objects) {
        result += fmt::format("    - {};\n", GetVulkanObjectDesc(object));
    }

    return result;
}


AM_MAYBE_UNUSED static std::vector<std::string> GetVulkanInstanceValidationLayers(const nlohmann::json& vkInstanceValidationLayersJson) noexcept
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    AM_LOG_GRAPHICS_API_INFO("Available Vulkan validation layers:\n{}", MakeVulkanObjectsListString(availableLayers));

    std::vector<std::string> requestedLayers = amjson::ParseJsonArray<std::string>(vkInstanceValidationLayersJson);
    
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


static std::vector<std::string> GetVulkanInstanceExtensions(const nlohmann::json& vkInstnceExtensionsJson) noexcept
{
    uint32_t availableVulkanExtCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableVulkanExtCount, nullptr);

    std::vector<VkExtensionProperties> availableVkExtensions(availableVulkanExtCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableVulkanExtCount, availableVkExtensions.data());

    AM_LOG_GRAPHICS_API_INFO("Available Vulkan instance extensions:\n{}", MakeVulkanObjectsListString(availableVkExtensions));

    std::vector<std::string> requestedExtensions = amjson::ParseJsonArray<std::string>(vkInstnceExtensionsJson);
    
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


static bool CheckVulkanLogicalDeviceExtensionSupport(const VulkanPhysicalDevice& physicalDevice, const std::vector<std::string>& requiredExtensions) noexcept
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice.pDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice.pDevice, nullptr, &extensionCount, availableExtensions.data());
    
    AM_LOG_GRAPHICS_API_INFO("Available Vulkan logical device extensions:\n{}", MakeVulkanObjectsListString(availableExtensions));

    for (const std::string& requiredExtension : requiredExtensions) {
        bool isLogicalDeviceExtensionSuppoted = false;
        
        for (const VkExtensionProperties& availableExtension : availableExtensions) {
            if (requiredExtension == availableExtension.extensionName) {
                isLogicalDeviceExtensionSuppoted = true;
                break;
            }
        }

        if (!isLogicalDeviceExtensionSuppoted) {
            AM_ASSERT_GRAPHICS_API(false, "Vulkan logical device doesn't support required extensions '{}'", requiredExtension.c_str());
            return false;
        }
    }

    return true;
}


static std::optional<VulkanAppInitInfo> ParseVulkanAppInitInfoJson(const fs::path& pathToJson) noexcept
{
    std::optional<nlohmann::json> jsonOpt = amjson::ParseJson(pathToJson);
    if (!jsonOpt.has_value()) {
        return {};
    }

    const nlohmann::json& window = jsonOpt.value()[JSON_APP_CONFIG_WINDOW_FIELD_NAME];

    VulkanAppInitInfo appInitInfo = {};
    window[JSON_APP_CONFIG_WINDOW_TITLE_FIELD_NAME].get_to(appInitInfo.windowInitInfo.title);
    window[JSON_APP_CONFIG_WINDOW_WIDTH_FIELD_NAME].get_to(appInitInfo.windowInitInfo.width);
    window[JSON_APP_CONFIG_WINDOW_HEIGHT_FIELD_NAME].get_to(appInitInfo.windowInitInfo.height);
    window[JSON_APP_CONFIG_WINDOW_RESIZABLE_FLAG_FIELD_NAME].get_to(appInitInfo.windowInitInfo.resizable);

    return appInitInfo;
}


static VulkanInstanceInitInfo ParseVulkanInstanceInitInfoJson(const nlohmann::json& vkInstanceJson) noexcept
{
    const nlohmann::json& buildJson         = GetVkInstanceOsBuildTypeSpecificJsonObj(vkInstanceJson);
    const nlohmann::json& extensionsJson    = buildJson[JSON_VK_CONFIG_INSTANCE_EXTENSIONS_FIELD_NAME];
    const nlohmann::json& layersJson        = buildJson[JSON_VK_CONFIG_INSTANCE_VALIDATION_LAYERS_FIELD_NAME];

    VulkanInstanceInitInfo instInitInfo = {};
    instInitInfo.extensionNames = GetVulkanInstanceExtensions(extensionsJson);
    instInitInfo.validationLayerNames = GetVulkanInstanceValidationLayers(layersJson);

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    const nlohmann::json& callbackJson = buildJson[JSON_VK_CONFIG_INSTANCE_CALLBACK_FIELD_NAME];
    
    instInitInfo.debugCallbackInitInfo = ParseVulkanCallbackInitInfoJson(callbackJson);
#endif

    return instInitInfo;
}


static VulkanPhysDeviceInitInfo ParseVulkanPhysDeviceInitInfoJson(const nlohmann::json& vkPhysDeviceJson) noexcept
{
    VulkanPhysDeviceInitInfo physDeviceInitInfo = {};

    const nlohmann::json& deviceTypesJson = vkPhysDeviceJson[JSON_VK_CONFIG_PHYS_DEVICE_TYPES_FIELD_NAME];
    physDeviceInitInfo.types = amjson::ParseJsonArray<std::string>(deviceTypesJson);

    return physDeviceInitInfo;
}


static VulkanLogicalDeviceInitInfo ParseVulkanLogicalDeviceInitInfoJson(const nlohmann::json& vkLogicalDeviceJson) noexcept
{
    VulkanLogicalDeviceInitInfo logicalDeviceInitInfo = {};

    const nlohmann::json& logicalDeviceExtensionsJson = vkLogicalDeviceJson[JSON_VK_CONFIG_LOGICAL_DEVICE_EXTENSIONS_FIELD_NAME];
    logicalDeviceInitInfo.extensionNames = amjson::ParseJsonArray<std::string>(logicalDeviceExtensionsJson);

    return logicalDeviceInitInfo;
}


static inline constexpr const ParsedVulkanPhysicalDeviceType* ConvertVkPhysicalDeviceTypeToParsedType(VkPhysicalDeviceType type) noexcept
{
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return &JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPE_DISCRETE;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return &JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPE_CPU;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return &JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPE_INTEGRATED;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return &JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPE_VIRTUAL;
            break;
        default:
            return nullptr;
    }
}


static uint64_t GetVulkanDeviceTypePriority(VkPhysicalDeviceType type, const std::vector<std::string>& requiredTypesStrs) noexcept
{
    const ParsedVulkanPhysicalDeviceType* parsedType = ConvertVkPhysicalDeviceTypeToParsedType(type);

    if (!parsedType) {
        return 0;
    }

    for (const std::string& requiredType : requiredTypesStrs) {
        if (requiredType == parsedType->str) {
            return parsedType->priority;
        }
    }

    return 0;
}


static void PrintPhysicalDeviceProperties(const VkPhysicalDeviceProperties& props) noexcept
{
#if defined(AM_LOGGING_ENABLED)
    const uint32_t deviceID = props.deviceID;
    const char* deviceName = props.deviceName;

    const ParsedVulkanPhysicalDeviceType* parsedType = ConvertVkPhysicalDeviceTypeToParsedType(props.deviceType);
    const char* deviceType = parsedType ? parsedType->str : "unknown";

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


static uint64_t GetVulkanPhysicalDevicePriority(const VulkanPhysicalDevice& device, const std::vector<std::string>& requiredTypes) noexcept
{
    if (device.pDevice == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE device.pDevice passed to {}", __FUNCTION__);
        return 0;
    }

    uint64_t priority = GetVulkanDeviceTypePriority(device.properties.deviceType, requiredTypes);

    AM_MAYBE_UNUSED const char* message = priority > 0 ? 
        AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Device '{}' with type '{}' match required types\n") :
        AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_RED_ASCII_CODE, "Device '{}' with type '{}' doesn't matches required types\n");
    AM_LOG_GRAPHICS_API_INFO(message, device.properties.deviceName, ConvertVkPhysicalDeviceTypeToParsedType(device.properties.deviceType)->str);
    
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

    std::optional<VulkanAppInitInfo> appInitInfoOpt = ParseVulkanAppInitInfoJson(paths::AM_PROJECT_CONFIG_FILE_PATH);
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
        s_pAppInst = nullptr;
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
}


bool VulkanApplication::IsInitialized() noexcept
{
    return s_pAppInst && s_pAppInst->IsInstanceInitialized();
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


#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
bool VulkanApplication::InitVulkanDebugCallback(const VulkanDebugCallbackInitInfo &initInfo) noexcept
{
    if (IsVulkanDebugCallbackInitialized()) {
        AM_LOG_WARN("Vulkan debug callback is already initialized");
        return true;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan debug message callback..."));

    VkDebugUtilsMessengerCreateInfoEXT createInfo = GetVkDebugUtilsMessengerCreateInfo(initInfo);

    if (CreateDebugUtilsMessengerEXT(s_pVulkanState->intance.pInstance, &createInfo, nullptr, &s_pVulkanState->intance.pDebugMessenger) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan debug callback initialization failed");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan debug message callback initialization finished..."));
    return true;
}


void VulkanApplication::TerminateVulkanDebugCallback() noexcept
{
    if (s_pVulkanState) {
        DestroyDebugUtilsMessengerEXT(s_pVulkanState->intance.pInstance, s_pVulkanState->intance.pDebugMessenger, nullptr);
    }
}
#endif


bool VulkanApplication::InitVulkanInstance(const VulkanInstanceInitInfo &initInfo) noexcept
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
    appInfo.pEngineName = VULKAN_ENGINE_NAME;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instCreateInfo = {};
    instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.pApplicationInfo = &appInfo;
    instCreateInfo.enabledLayerCount = 0;
    instCreateInfo.ppEnabledLayerNames = nullptr;
    instCreateInfo.ppEnabledExtensionNames = nullptr;

    const std::vector<std::string>& extensionNames = initInfo.extensionNames;
    const size_t extensionsCount = extensionNames.size();

    if (extensionsCount > 0) {
        s_pVulkanState->intance.extensions.resize(extensionsCount);
        
        CopyStringArrayToCharPtrArray(s_pVulkanState->intance.extensions.data(), extensionNames.data(), extensionsCount);
        
        instCreateInfo.ppEnabledExtensionNames = s_pVulkanState->intance.extensions.data();
    }

    instCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsCount);
    
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    const std::vector<std::string>& validationLayerNames = initInfo.validationLayerNames;
    const size_t validationLayersCount = validationLayerNames.size();
 
    if (validationLayersCount > 0) {
        s_pVulkanState->intance.validationLayers.resize(validationLayersCount);
        
        CopyStringArrayToCharPtrArray(s_pVulkanState->intance.validationLayers.data(), validationLayerNames.data(), validationLayersCount);
        
        instCreateInfo.ppEnabledLayerNames = s_pVulkanState->intance.validationLayers.data();
    }

    instCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersCount);
    
    VkDebugUtilsMessengerCreateInfoEXT vulkanDebugMessangerCreateInfo = GetVkDebugUtilsMessengerCreateInfo(initInfo.debugCallbackInitInfo);
    instCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&vulkanDebugMessangerCreateInfo;
#endif

    if (vkCreateInstance(&instCreateInfo, nullptr, &s_pVulkanState->intance.pInstance) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan instance creation failed");
        return false;
    }

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    if (!InitVulkanDebugCallback(initInfo.debugCallbackInitInfo)) {
        return false;
    }
#endif

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan instance initialization finished"));
    return true;
}


void VulkanApplication::TerminateVulkanInstance() noexcept
{
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    TerminateVulkanDebugCallback();
#endif

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


bool VulkanApplication::InitVulkanPhysicalDevice(const VulkanPhysDeviceInitInfo &initInfo) noexcept
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

        const uint64_t devicePriority = GetVulkanPhysicalDevicePriority(physDevice, initInfo.types);

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


bool VulkanApplication::InitVulkanLogicalDevice(const VulkanLogicalDeviceInitInfo &initInfo) noexcept
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

    if (!CheckVulkanLogicalDeviceExtensionSupport(physicalDevice, initInfo.extensionNames)) {
        return false;
    }

    AM_LOG_GRAPHICS_API_INFO("Included Vulkan logical device extensions:\n{}", MakeVulkanObjectsListString(initInfo.extensionNames));

    std::vector<VkDeviceQueueCreateInfo> deviceQueueInfos(queueFamilies.size());

    for (size_t i = 0; i < queueFamilies.size(); ++i) {
        deviceQueueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueInfos[i].queueFamilyIndex = queueFamilies[i].index.value();
        deviceQueueInfos[i].queueCount = 1;
        deviceQueueInfos[i].pQueuePriorities = &queueFamilies[i].priority;
    }

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pEnabledFeatures = &physicalDevice.features;
    createInfo.pQueueCreateInfos = deviceQueueInfos.data();
    createInfo.queueCreateInfoCount = deviceQueueInfos.size();
    
// #if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
//     createInfo.enabledLayerCount = s_pVulkanState->intance.validationLayers.size();
//     createInfo.ppEnabledLayerNames = createInfo.enabledLayerCount ? s_pVulkanState->intance.validationLayers.data() : nullptr;
// #endif

    const std::vector<std::string>& logicalDeviceExtensionNames = initInfo.extensionNames;
    const size_t extensionsCount = logicalDeviceExtensionNames.size();

    if (extensionsCount > 0) {
        s_pVulkanState->logicalDevice.extensions.resize(extensionsCount);
        
        CopyStringArrayToCharPtrArray(s_pVulkanState->logicalDevice.extensions.data(), logicalDeviceExtensionNames.data(), extensionsCount);
        
        createInfo.ppEnabledExtensionNames = s_pVulkanState->logicalDevice.extensions.data();
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsCount);
    
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

    std::vector<VkImageView>& swapChainImageViews = s_pVulkanState->swapChain.swapChainImageViews;
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
        for (VkImageView& imageView : s_pVulkanState->swapChain.swapChainImageViews) {
            vkDestroyImageView(s_pVulkanState->logicalDevice.pDevice, imageView, nullptr);
        }

        vkDestroySwapchainKHR(s_pVulkanState->logicalDevice.pDevice, s_pVulkanState->swapChain.pSwapChain, nullptr);
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
        AM_ASSERT(false, "VulkanLogicalDevice must be initialized before graphics pipeline initialization");
        return false;
    }

    if (!IsVulkanSwapChainInitialized()) {
        AM_ASSERT(false, "VulkanSwapChain must be initialized before graphics pipeline initialization");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan graphics pipeline..."));

    VulkanShaderSystem& shaderSystem = VulkanShaderSystem::Instance();
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = shaderSystem.m_shaderModuleGroups[0].modules[VulkanShaderKind_VERTEX].pModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo pixShaderStageInfo = {};
    pixShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pixShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pixShaderStageInfo.module = shaderSystem.m_shaderModuleGroups[0].modules[VulkanShaderKind_PIXEL].pModule;
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

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkDevice pLogicalDevice = s_pVulkanState->logicalDevice.pDevice;
    VkPipelineLayout& pPipelineLayout = s_pVulkanState->graphicsPipeline.pLayout;

    if (vkCreatePipelineLayout(pLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &pPipelineLayout) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan pipeline creation failed");
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan graphics pipeline initialization finished"));

    return true;
}


void VulkanApplication::TerminateVulkanGraphicsPipeline() noexcept
{
    if (s_pVulkanState) {
        vkDestroyPipelineLayout(s_pVulkanState->logicalDevice.pDevice, s_pVulkanState->graphicsPipeline.pLayout, nullptr);
    }
}


bool VulkanApplication::InitVulkan() noexcept
{
    if (IsVulkanInitialized()) {
        AM_LOG_WARN("Vulkan is already initialized");
        return true;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, "Initializing Vulkan..."));

    s_pVulkanState = std::make_unique<VulkanState>();

    const std::optional<nlohmann::json> vulkanConfigOpt = amjson::ParseJson(paths::AM_VULKAN_CONFIG_FILE_PATH);
    if (!vulkanConfigOpt.has_value()) {
        AM_ASSERT(false, "Failed to parse Vulkan config file");
        return false;
    }

    const nlohmann::json& vulkanConfigJson = vulkanConfigOpt.value();
    const nlohmann::json& instanceConfigJson = vulkanConfigJson[JSON_VK_CONFIG_INSTANCE_FIELD_NAME];
    const nlohmann::json& physDeviceConfigJson = vulkanConfigJson[JSON_VK_CONFIG_PHYS_DEVICE_FIELD_NAME];
    const nlohmann::json& logicalDeviceConfigJson = vulkanConfigJson[JSON_VK_CONFIG_LOGICAL_DEVICE_FIELD_NAME];

    if (!InitVulkanInstance(ParseVulkanInstanceInitInfoJson(instanceConfigJson))) {
        return false;
    }

    if (!InitVulkanSurface()) {
        return false;
    }
    
    if (!InitVulkanPhysicalDevice(ParseVulkanPhysDeviceInitInfoJson(physDeviceConfigJson))) {
        return false;
    }

    if (!InitVulkanLogicalDevice(ParseVulkanLogicalDeviceInitInfoJson(logicalDeviceConfigJson))) {
        return false;
    }

    if (!VulkanShaderSystem::Init(s_pVulkanState->logicalDevice.pDevice)) {
        return false;
    }

    if (!InitVulkanSwapChain()) {
        return false;
    }

    if (!InitVulkanGraphicsPipeline()) {
        return false;
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Vulkan initialization finished"));

    return true;
}


void VulkanApplication::TerminateVulkan() noexcept
{
    TerminateVulkanGraphicsPipeline();
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


bool VulkanApplication::IsVulkanInstanceInitialized() noexcept
{
    return s_pVulkanState && s_pVulkanState->intance.pInstance != VK_NULL_HANDLE;
}


bool VulkanApplication::IsVulkanSurfaceInitialized() noexcept
{
    return s_pVulkanState && s_pVulkanState->surface.pSurface != VK_NULL_HANDLE;
}


bool VulkanApplication::IsVulkanDebugCallbackInitialized() noexcept
{
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    return s_pVulkanState && s_pVulkanState->intance.pDebugMessenger != VK_NULL_HANDLE;
#else
    return true;
#endif
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

    if (swapChain.swapChainImageViews.size() != swapChain.images.size() || swapChain.swapChainImageViews.empty()) {
        return false;
    }

    for (const VkImageView& imageView : swapChain.swapChainImageViews) {
        if (imageView == VK_NULL_HANDLE) {
            return false;
        }
    }

    return true;
}

bool VulkanApplication::IsVulkanGraphicsPipelineInitialized() noexcept
{
    return s_pVulkanState && s_pVulkanState->graphicsPipeline.pLayout != VK_NULL_HANDLE;
}


bool VulkanApplication::IsVulkanInitialized() noexcept
{
    return IsVulkanInstanceInitialized() 
        && IsVulkanDebugCallbackInitialized() 
        && IsVulkanSurfaceInitialized()
        && IsVulkanPhysicalDeviceInitialized()
        && IsVulkanLogicalDeviceInitialized()
        && IsVulkanSwapChainInitialized()
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