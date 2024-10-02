#include "../pch.h"
#include "vk_application.h"


static constexpr const char* VULKAN_ENGINE_NAME = "Vulkan Engine";

static constexpr const char* JSON_VK_CONFIG_OS_FIELD_NAME = "os";
static constexpr const char* JSON_VK_CONFIG_BUILD_TYPE_FIELD_NAME = "build_type";

#if defined(AM_OS_WINDOWS)
    static constexpr const char* JSON_VK_CONFIG_OS_NAME = "win32";
#else
    #error Currently, only Windows is supported
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
static constexpr const char* JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPES_FIELD_NAME = "types";


// Vulkan logical device config specific field names
static constexpr const char* JSON_VK_CONFIG_LOGICAL_DEVICE_FIELD_NAME = "logical_device";


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
    1.0f
};
    
static constexpr std::array<VkQueueFlagBits, VulkanQueueFamilies::RequiredQueueFamilyType_COUNT> REQUIRED_QUIE_FAMILIES_FLAGS = {
    VK_QUEUE_GRAPHICS_BIT
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

    static const auto ConvertStrToSeverityBits = [](const std::string& severity) -> uint32_t
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
            AM_ASSERT(false, "Invalid Vulkan callback message severity JSON tokken {}. Must be one of:\n\t- {};\n\t- {};\n\t- {};\n\t- {};\n\t- {};", 
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

    static const auto ConvertStrToTypeBits = [](const std::string& type) -> uint32_t
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
            AM_ASSERT(false, "Invalid Vulkan callback message type JSON tokken {}. Must be one of:\n\t- {};\n\t- {};\n\t- {};\n\t- {};", 
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


static VulkanDebugCallbackInitInfo ParseVulkanCallbackInitInfoJson(const nlohmann::json& vkInstanceJson) noexcept
{
    VulkanDebugCallbackInitInfo result = {};
    result.pCallback = VulkanDebugCallback;

    const nlohmann::json& build = GetVkInstanceOsBuildTypeSpecificJsonObj(vkInstanceJson);
    const nlohmann::json& callback = build[JSON_VK_CONFIG_INSTANCE_CALLBACK_FIELD_NAME];

    const nlohmann::json& severity = callback[JSON_VK_CONFIG_INSTANCE_CALLBACK_SEVERITY_FIELD_NAME];
    std::vector<std::string> severityTokens = amjson::ParseArrayJson<std::string>(severity);
    result.messageSeverity = GetVkDebugUtilsMessageSeverityFlagBits(severityTokens);

    const nlohmann::json& messageTypes = callback[JSON_VK_CONFIG_INSTANCE_CALLBACK_MESSAGE_TYPES_FIELD_NAME];
    std::vector<std::string> messageTypeTokens = amjson::ParseArrayJson<std::string>(messageTypes);
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


AM_MAYBE_UNUSED static inline std::string GetVulkanValidationLayerDesc(const VkLayerProperties& layer) noexcept
{
    return fmt::format("{} -> {}{}{}", layer.layerName, AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, layer.description, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
}

AM_MAYBE_UNUSED static inline std::string GetVulkanValidationLayerDesc(const std::string& layerDesc) noexcept
{
    return fmt::format("{}{}{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, layerDesc, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
}

template<typename ValidationLayerT>
AM_MAYBE_UNUSED static std::string MakeVulkanValidationLayersListString(const std::vector<ValidationLayerT>& layers) noexcept
{
    std::string result = "";
        
    for (const ValidationLayerT& layer : layers) {
        result += fmt::format("{}{}{}", "\t- ", GetVulkanValidationLayerDesc(layer), ";\n");
    }

    return result;
}


AM_MAYBE_UNUSED static inline std::string GetVulkanExtensionDesc(const VkExtensionProperties& extension) noexcept
{
    return fmt::format("{}{}{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, extension.extensionName, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
}

AM_MAYBE_UNUSED static inline std::string GetVulkanExtensionDesc(const std::string& extension) noexcept
{
    return fmt::format("{}{}{}", AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, extension, AM_OUTPUT_COLOR_RESET_ASCII_CODE);
}

template<typename ExtensionT>
AM_MAYBE_UNUSED static std::string MakeVulkanExtensionsListString(const std::vector<ExtensionT>& extensions) noexcept
{
    std::string result = "";
        
    for (const ExtensionT& extension : extensions) {
        result += fmt::format("{}{}{}", "\t- ", GetVulkanExtensionDesc(extension), ";\n");
    }

    return result;
}


AM_MAYBE_UNUSED static std::vector<std::string> GetVulkanValidationLayers(const nlohmann::json& vkInstanceJson) noexcept
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    AM_LOG_GRAPHICS_API_INFO("Available Vulkan validation layers:\n{}", MakeVulkanValidationLayersListString(availableLayers));

    const nlohmann::json& build = GetVkInstanceOsBuildTypeSpecificJsonObj(vkInstanceJson);
    const nlohmann::json& layers = build[JSON_VK_CONFIG_INSTANCE_VALIDATION_LAYERS_FIELD_NAME];
    std::vector<std::string> requestedLayers = amjson::ParseArrayJson<std::string>(layers);
    
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
    AM_LOG_GRAPHICS_API_INFO("Included Vulkan validation layers:\n{}", MakeVulkanValidationLayersListString(result));

    return result;
}


static std::vector<std::string> GetVulkanExtensions(const nlohmann::json& vkInstanceJson) noexcept
{
    uint32_t availableVulkanExtCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableVulkanExtCount, nullptr);

    std::vector<VkExtensionProperties> availableVkExtensions(availableVulkanExtCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableVulkanExtCount, availableVkExtensions.data());

    AM_LOG_GRAPHICS_API_INFO("Available Vulkan extensions:\n{}", MakeVulkanExtensionsListString(availableVkExtensions));

    const nlohmann::json& build = GetVkInstanceOsBuildTypeSpecificJsonObj(vkInstanceJson);
    const nlohmann::json& extensions = build[JSON_VK_CONFIG_INSTANCE_EXTENSIONS_FIELD_NAME];
    std::vector<std::string> requestedExtensions = amjson::ParseArrayJson<std::string>(extensions);
    
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
    AM_LOG_GRAPHICS_API_INFO("Included Vulkan extensions:\n{}", MakeVulkanExtensionsListString(result));

    return result;
}


static std::optional<VulkanAppInitInfo> ParseVulkanAppInitInfoJson(const std::filesystem::path& pathToJson) noexcept
{
    std::optional<nlohmann::json> jsonOpt = amjson::ParseJson(pathToJson);
    if (!jsonOpt.has_value()) {
        return {};
    }

    const nlohmann::json& build = GetVkInstanceOsBuildTypeSpecificJsonObj(jsonOpt.value());
    const nlohmann::json& window = build[JSON_APP_CONFIG_WINDOW_FIELD_NAME];

    VulkanAppInitInfo appInitInfo = {};
    window[JSON_APP_CONFIG_WINDOW_TITLE_FIELD_NAME].get_to(appInitInfo.title);
    window[JSON_APP_CONFIG_WINDOW_WIDTH_FIELD_NAME].get_to(appInitInfo.width);
    window[JSON_APP_CONFIG_WINDOW_HEIGHT_FIELD_NAME].get_to(appInitInfo.height);
    window[JSON_APP_CONFIG_WINDOW_RESIZABLE_FLAG_FIELD_NAME].get_to(appInitInfo.resizable);

    return appInitInfo;
}


static VulkanInstanceInitInfo ParseVulkanInstanceInitInfoJson(const nlohmann::json& vkInstanceJson) noexcept
{
    VulkanInstanceInitInfo instInitInfo = {};
    instInitInfo.extensionNames = GetVulkanExtensions(vkInstanceJson);
    instInitInfo.validationLayerNames = GetVulkanValidationLayers(vkInstanceJson);

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    instInitInfo.debugCallbackInitInfo = ParseVulkanCallbackInitInfoJson(vkInstanceJson);
#endif

    return instInitInfo;
}


static VulkanPhysDeviceInitInfo ParseVulkanPhysDeviceInitInfoJson(const nlohmann::json& vkPhysDeviceJson) noexcept
{
    VulkanPhysDeviceInitInfo deviceInitInfo = {};

    const nlohmann::json& deviceTypesJson = vkPhysDeviceJson[JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPES_FIELD_NAME];
    deviceInitInfo.types = amjson::ParseArrayJson<std::string>(deviceTypesJson);

    return deviceInitInfo;
}


static VulkanLogicalDeviceInitInfo ParseVulkanLogicalDeviceInitInfoJson(const nlohmann::json& vkPhysDeviceJson) noexcept
{
    return {};
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

    constexpr const char* format = "Device {} properties:\n" AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_YELLOW_ASCII_CODE, 
        "  - Name: {};\n  - Type: {};\n  - API Version: {}.{}.{};\n  - Driver Version: {}.{}.{}."
    );

    AM_LOG_GRAPHICS_API_INFO(format, deviceID, deviceName, deviceType, apiVersionMajor, apiVersionMinor, apiVersionPatch, driverVersionMajor, driverVersionMinor, driverVersionPatch);
#endif
}


static void PrintPhysicalDeviceFeatures(const VkPhysicalDeviceFeatures& features) noexcept
{
#if defined(AM_LOGGING_ENABLED)
    
#endif
}


static VulkanQueueFamilies FindRequiredVulkanQueueFamilies(VkPhysicalDevice pPhysicalDevice) noexcept
{
    if (pPhysicalDevice == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE pPhysicalDevice passed to {}", __FUNCTION__);
        return {};
    }

    VulkanQueueFamilies families = {};

    uint32_t familiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevice, &familiesCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(familiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevice, &familiesCount, queueFamilies.data());

    auto& descs = families.descs;
    for (uint32_t requiredFlagBitsIndex = 0; requiredFlagBitsIndex < descs.size(); ++requiredFlagBitsIndex) {
        for (uint32_t familyIndex = 0; familyIndex < queueFamilies.size(); ++familyIndex) {
            const VkQueueFlags& queueFlags = queueFamilies[familyIndex].queueFlags;

            if (queueFlags & descs[requiredFlagBitsIndex].flags) {
                descs[requiredFlagBitsIndex].index = familyIndex;
                break;
            }
        }
    }

    return families;
}


static VulkanPhysicalDevice GetVulkanPhysicalDeviceInternal(VkPhysicalDevice pPhysicalDevice) noexcept
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

    device.queueFamilies = FindRequiredVulkanQueueFamilies(pPhysicalDevice);

    return device;
}


static uint64_t GetVulkanPhysicalDevicePriority(const VulkanPhysicalDevice& device, const VulkanPhysDeviceInitInfo& requirements) noexcept
{
    if (device.pDevice == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE device.pDevice passed to {}", __FUNCTION__);
        return 0;
    }

    uint64_t priority = GetVulkanDeviceTypePriority(device.properties.deviceType, requirements.types);

    AM_MAYBE_UNUSED const char* message = priority > 0 ? 
        AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Device '{}' with type '{}' match required types\n") :
        AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_RED_ASCII_CODE, "Device '{}' with type '{}' doesn't matches required types\n");
    AM_LOG_GRAPHICS_API_INFO(message, device.properties.deviceName, ConvertVkPhysicalDeviceTypeToParsedType(device.properties.deviceType)->str);

    return priority;
}


VulkanQueueFamilies::VulkanQueueFamilies()
    : descs(RequiredQueueFamilyType_COUNT)
{
    for (size_t i = 0; i < RequiredQueueFamilyType_COUNT; ++i) {
        descs[i].priority = FAMILY_PRIORITIES[i];
        descs[i].flags = REQUIRED_QUIE_FAMILIES_FLAGS[i];
    }
}


bool VulkanQueueFamilies::IsComplete() const noexcept
{
    return std::find_if(descs.cbegin(), descs.cend(), [](const auto& desc) {
        return !desc.index.has_value();
    }) == descs.cend();
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

#if defined(AM_LOGGING_ENABLED)
    glfwSetErrorCallback([](int errorCode, const char* description) -> void {
        AM_LOG_WINDOW_ERROR("{} (code: {})", description, errorCode);
    });
#endif
    
    if (glfwInit() != GLFW_TRUE) {
        AM_ASSERT_WINDOW(false, "GLFW initialization failed");
        return false;
    }

    if (!InitVulkan()) {
        Terminate();
        return false;
    }

    s_pAppInst = std::unique_ptr<VulkanApplication>(new VulkanApplication(appInitInfo));
    if (!s_pAppInst) {
        AM_ASSERT(false, "VulkanApplication instance allocation failed");
        Terminate();
        return false;
    }

    if (!s_pAppInst->IsInstanceInitialized()) {
        AM_ASSERT(false, "VulkanApplication instance initialization failed");
        Terminate();
        return false;
    }

    return true;
}


void VulkanApplication::Terminate() noexcept
{
    s_pAppInst = nullptr;

    TerminateVulkan();
    glfwTerminate();
}


bool VulkanApplication::IsInitialized() noexcept
{
    return s_pAppInst && s_pAppInst->IsInstanceInitialized();
}


void VulkanApplication::Run() noexcept
{
    while(!glfwWindowShouldClose(m_glfwWindow)) {
        glfwPollEvents();
    }
}


#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
bool VulkanApplication::InitVulkanDebugCallback(const VulkanDebugCallbackInitInfo &initInfo) noexcept
{
    if (IsVulkanDebugCallbackInitialized()) {
        AM_LOG_WARN("Vulkan debug callback is already initialized");
        return true;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo = GetVkDebugUtilsMessengerCreateInfo(initInfo);

    if (CreateDebugUtilsMessengerEXT(s_pVulkanState->intance.pInstance, &createInfo, nullptr, &s_pVulkanState->intance.pDebugMessenger) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan debug callback initialization failed");
        return false;
    }

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

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Default Vulkan Application";
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


bool VulkanApplication::InitVulkanPhysicalDevice(const VulkanPhysDeviceInitInfo& initInfo) noexcept
{
    if (IsVulkanPhysicalDeviceInitialized()) {
        AM_LOG_WARN("Vulkan physical device is already initialized");
        return true;
    }

    if (!IsVulkanInstanceInitialized()) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan instance must be initialized before physical device initialization");
        return false;
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(s_pVulkanState->intance.pInstance, &deviceCount, nullptr);

    if (deviceCount <= 0) {
        AM_ASSERT_GRAPHICS_API(false, "There are no any physical graphics devices that support Vulkan");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(s_pVulkanState->intance.pInstance, &deviceCount, devices.data());

    AM_LOG_GRAPHICS_API_INFO("Picking suitable physical physicalDevice...");

    std::multimap<uint64_t, VulkanPhysicalDevice> suitableDevices;
    
    for (VkPhysicalDevice pPhysicalDevice : devices) {
        VulkanPhysicalDevice desc = GetVulkanPhysicalDeviceInternal(pPhysicalDevice);

        const uint64_t devicePriority = GetVulkanPhysicalDevicePriority(desc, initInfo);
        
        if (devicePriority) {
            suitableDevices.insert(std::make_pair(devicePriority, desc));
        }
    }

    if (suitableDevices.empty()) {
        AM_ASSERT_GRAPHICS_API(false, "There are no any Vulkan physical graphics devices that support required properties");
        return false;
    }

    for (auto it = suitableDevices.rbegin(); it != suitableDevices.rend(); ++it) {
        if (it->second.queueFamilies.IsComplete()) {
            s_pVulkanState->physicalDevice = std::move(it->second);
            break;
        }
    }

    if (s_pVulkanState->physicalDevice.pDevice == VK_NULL_HANDLE) {
        AM_ASSERT_GRAPHICS_API(false, "VkPhysicalDevice is VK_NULL_HANDLE");
        return false;
    }

    if (!s_pVulkanState->physicalDevice.queueFamilies.IsComplete()) {
        AM_ASSERT_GRAPHICS_API(false, "There are no any Vulkan physical device that support required queue families");
        return false;
    }

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

    const auto& physicalDevice = s_pVulkanState->physicalDevice;
    const auto& queueFamilies = s_pVulkanState->physicalDevice.queueFamilies.descs;

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
    // createInfo.enabledExtensionCount = s_pVulkanState->intance.extensions.size();
    // createInfo.ppEnabledExtensionNames = createInfo.enabledExtensionCount ? s_pVulkanState->intance.extensions.data() : nullptr;
    // createInfo.enabledLayerCount = s_pVulkanState->intance.validationLayers.size();
    // createInfo.ppEnabledLayerNames = createInfo.enabledLayerCount ? s_pVulkanState->intance.validationLayers.data() : nullptr;

    if (vkCreateDevice(s_pVulkanState->physicalDevice.pDevice, &createInfo, nullptr, &s_pVulkanState->logicalDevice.pDevice) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan logical device creation failed");
        return false;
    }

    VkDevice pDevice = s_pVulkanState->logicalDevice.pDevice;
    
    s_pVulkanState->logicalDevice.queues.resize(VulkanQueueFamilies::RequiredQueueFamilyType_COUNT);
    for (uint32_t i = 0; i < VulkanQueueFamilies::RequiredQueueFamilyType_COUNT; ++i) {
        const uint32_t queueFamilyIndex = s_pVulkanState->physicalDevice.queueFamilies.descs[i].index.value();
        VkQueue* ppQueue = &s_pVulkanState->logicalDevice.queues[i];

        vkGetDeviceQueue(pDevice, queueFamilyIndex, i, ppQueue);

        if (*ppQueue == VK_NULL_HANDLE) {
            AM_ASSERT_GRAPHICS_API(false, "Failed to get device queue");
            return false;
        }
    }

    return true;
}


void VulkanApplication::TerminateVulkanLogicalDevice() noexcept
{
    if (s_pVulkanState) {
        vkDestroyDevice(s_pVulkanState->logicalDevice.pDevice, nullptr);
    }
}


bool VulkanApplication::InitVulkan() noexcept
{
    if (IsVulkanInitialized()) {
        AM_LOG_WARN("Vulkan is already initialized");
        return true;
    }

    s_pVulkanState = std::make_unique<VulkanState>();

    const std::optional<nlohmann::json> vulkanConfigOpt = amjson::ParseJson(paths::AM_VULKAN_CONFIG_FILE_PATH);
    if (!vulkanConfigOpt.has_value()) {
        AM_ASSERT(false, "Failed to parse Vulkan config file");
        return false;
    }

    const nlohmann::json& vulkanConfigJson = vulkanConfigOpt.value();
    const nlohmann::json& instanceConfigJson = vulkanConfigJson[JSON_VK_CONFIG_INSTANCE_FIELD_NAME];
    const nlohmann::json& physDeviceConfigJson = vulkanConfigJson[JSON_VK_CONFIG_PHYS_DEVICE_FIELD_NAME];
    const nlohmann::json& logicalDeviceConfigJson = vulkanConfigJson[JSON_VK_CONFIG_PHYS_DEVICE_FIELD_NAME];

    if (!InitVulkanInstance(ParseVulkanInstanceInitInfoJson(instanceConfigJson))) {
        return false;
    }
    
    if (!InitVulkanPhysicalDevice(ParseVulkanPhysDeviceInitInfoJson(physDeviceConfigJson))) {
        return false;
    }

    if (!InitVulkanLogicalDevice(ParseVulkanLogicalDeviceInitInfoJson(logicalDeviceConfigJson))) {
        return false;
    }

    return true;
}


void VulkanApplication::TerminateVulkan() noexcept
{
    TerminateVulkanLogicalDevice();
    TerminateVulkanPhysicalDevice();
    TerminateVulkanInstance();

    s_pVulkanState = nullptr;
}


bool VulkanApplication::IsVulkanInstanceInitialized() noexcept
{
    return s_pVulkanState && s_pVulkanState->intance.pInstance != VK_NULL_HANDLE;
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


bool VulkanApplication::IsVulkanInitialized() noexcept
{
    return IsVulkanDebugCallbackInitialized() 
        && IsVulkanDebugCallbackInitialized() 
        && IsVulkanPhysicalDeviceInitialized()
        && IsVulkanLogicalDeviceInitialized();
}


VulkanApplication::VulkanApplication(const VulkanAppInitInfo &appInitInfo)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, appInitInfo.resizable ? GLFW_TRUE : GLFW_FALSE);

    m_glfwWindow = glfwCreateWindow((int)appInitInfo.width, (int)appInitInfo.height, appInitInfo.title.c_str(), nullptr, nullptr);
}


bool VulkanApplication::IsInstanceInitialized() const noexcept
{
    return IsGlfwWindowCreated();
}


bool VulkanApplication::IsGlfwWindowCreated() const noexcept
{
    return m_glfwWindow != nullptr;
}


VulkanApplication::~VulkanApplication()
{
    glfwDestroyWindow(m_glfwWindow);
}