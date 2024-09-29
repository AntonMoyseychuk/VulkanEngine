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

static constexpr const char* JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_FIELD_NAME = "requirements";
static constexpr const char* JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPES_FIELD_NAME = "types";

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


AM_MAYBE_UNUSED static constexpr size_t MAX_VULKAN_EXTENSIONS_COUNT = 64u;
AM_MAYBE_UNUSED static constexpr size_t MAX_VULKAN_VALIDATION_LAYERS_COUNT = 64u;


struct VulkanApplication::VulkanAppInitInfo
{
    std::string title = {};

    uint32_t width = 0;
    uint32_t height = 0;
    
    bool resizable = false;
};


#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
struct VulkanApplication::VulkanDebugCallbackInitInfo
{
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity;
    VkDebugUtilsMessageTypeFlagsEXT messageType;
    PFN_vkDebugUtilsMessengerCallbackEXT pCallback;
};
#endif


struct VulkanApplication::VulkanInstanceInitInfo
{
    std::vector<std::string> extensionNames;
    std::vector<std::string> validationLayerNames;

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    VulkanDebugCallbackInitInfo debugCallbackInitInfo;
#endif
};


struct VulkanApplication::VulkanPhysDeviceInitInfo
{
    std::vector<std::string> types;
};


using VulkanAppInitInfo = VulkanApplication::VulkanAppInitInfo;
using VulkanInstanceInitInfo = VulkanApplication::VulkanInstanceInitInfo;
using VulkanPhysDeviceInitInfo = VulkanApplication::VulkanPhysDeviceInitInfo;

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
using VulkanDebugCallbackInitInfo = VulkanApplication::VulkanDebugCallbackInitInfo;
#endif


struct VulkanQueueFamiliesIndices
{
    enum RequiredQueueFamilyType
    {
        RequiredQueueFamilyType_GRAPHICS,
        RequiredQueueFamilyType_COUNT
    };

    VulkanQueueFamiliesIndices()
        : familiesIds(RequiredQueueFamilyType_COUNT)
    {
    }

    bool IsComplete() const noexcept
    {
        return std::find_if(familiesIds.cbegin(), familiesIds.cend(), [](const auto& familyOpt) {
            return !familyOpt.has_value();
        }) == familiesIds.cend();
    }

    static constexpr inline std::array<VkQueueFlagBits, RequiredQueueFamilyType_COUNT> REQUIRED_QUIE_FAMILIES_FLAGS = {
        VK_QUEUE_GRAPHICS_BIT
    };

    std::vector<std::optional<uint32_t>> familiesIds;
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


// Potentialy unsafe, since lifetime of chPtrArr must be less of equal than strArr
static void MapArrayOfStringToArrayOfCharPtrs(const char** chPtrArr, size_t& inoutChPtrArrSize, const std::vector<std::string>& strArr) noexcept
{
    AM_ASSERT(strArr.size() <= inoutChPtrArrSize, "Target chPtrArr size is less than source strArr");

    inoutChPtrArrSize = strArr.size();
    for(size_t i = 0; i < inoutChPtrArrSize; ++i) {
        chPtrArr[i] = strArr[i].data();
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

    const nlohmann::json& requirementsJson = vkPhysDeviceJson[JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_FIELD_NAME];
    
    const nlohmann::json& deviceTypesJson = requirementsJson[JSON_VK_CONFIG_PHYS_DEVICE_REQIUREMENTS_TYPES_FIELD_NAME];
    deviceInitInfo.types = amjson::ParseArrayJson<std::string>(deviceTypesJson);

    return deviceInitInfo;
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


static VulkanQueueFamiliesIndices FindRequiredVulkanQueueFamilies(VkPhysicalDevice device) noexcept
{
    if (device == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE device passed to {}", __FUNCTION__);
        return {};
    }

    VulkanQueueFamiliesIndices indices = {};

    uint32_t familiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familiesCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(familiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familiesCount, queueFamilies.data());

    for (uint32_t requiredFlagBitsId = 0; requiredFlagBitsId < VulkanQueueFamiliesIndices::REQUIRED_QUIE_FAMILIES_FLAGS.size(); ++requiredFlagBitsId) {
        const VkQueueFlags& requiredQueueFlags = VulkanQueueFamiliesIndices::REQUIRED_QUIE_FAMILIES_FLAGS[requiredFlagBitsId];

        for (uint32_t familyId = 0; familyId < queueFamilies.size(); ++familyId) {
            const VkQueueFlags& queueFlags = queueFamilies[familyId].queueFlags;

            if (queueFlags & requiredQueueFlags) {
                indices.familiesIds[requiredFlagBitsId] = familyId;
                break;
            }
        }
    }

    return indices;
}


static uint64_t GetVulkanPhysicalDevicePriority(VkPhysicalDevice device, const VulkanPhysDeviceInitInfo& requirements) noexcept
{
    if (device == VK_NULL_HANDLE) {
        AM_LOG_GRAPHICS_API_WARN("VK_NULL_HANDLE device passed to {}", __FUNCTION__);
        return 0;
    }

    VkPhysicalDeviceProperties deviceProps = {};
    vkGetPhysicalDeviceProperties(device, &deviceProps);

    PrintPhysicalDeviceProperties(deviceProps);
    
    uint64_t priority = GetVulkanDeviceTypePriority(deviceProps.deviceType, requirements.types);
    
    AM_MAYBE_UNUSED const char* message = priority > 0 ? 
        AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Device '{}' with type '{}' match required types\n") :
        AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_RED_ASCII_CODE, "Device '{}' with type '{}' doesn't matches required types\n");
    AM_LOG_GRAPHICS_API_INFO(message, deviceProps.deviceName, ConvertVkPhysicalDeviceTypeToParsedType(deviceProps.deviceType)->str);

    {
        // Will be used in the future, may be
        VkPhysicalDeviceFeatures deviceFeatures = {};
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        PrintPhysicalDeviceFeatures(deviceFeatures);
    }



    return priority;
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

    if (CreateDebugUtilsMessengerEXT(s_vulkanState.instance, &createInfo, nullptr, &s_vulkanState.debugMessenger) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan debug callback initialization failed");
        return false;
    }

    return true;
}


void VulkanApplication::TerminateVulkanDebugCallback() noexcept
{
    DestroyDebugUtilsMessengerEXT(s_vulkanState.instance, s_vulkanState.debugMessenger, nullptr);
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

    const std::vector<std::string>& extensionNames = initInfo.extensionNames;
    const char* extensionNamesChar[MAX_VULKAN_EXTENSIONS_COUNT] = {};
    size_t extensionsCount = extensionNames.size();

    if (extensionsCount > 0) {
        MapArrayOfStringToArrayOfCharPtrs(extensionNamesChar, extensionsCount, extensionNames);
        instCreateInfo.ppEnabledExtensionNames = extensionNamesChar;
    }

    instCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsCount);
    
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    const std::vector<std::string>& validationLayerNames = initInfo.validationLayerNames;
    const char* validationLayerNamesChar[MAX_VULKAN_VALIDATION_LAYERS_COUNT] = {};
    size_t validationLayersCount = validationLayerNames.size();
 
    if (validationLayersCount > 0) {
        MapArrayOfStringToArrayOfCharPtrs(validationLayerNamesChar, validationLayersCount, validationLayerNames);
        instCreateInfo.ppEnabledLayerNames = validationLayerNamesChar;
    }

    instCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersCount);

    const VulkanDebugCallbackInitInfo& debugCallbackInitInfo = initInfo.debugCallbackInitInfo;
    
    VkDebugUtilsMessengerCreateInfoEXT vulkanDebugMessangerCreateInfo = GetVkDebugUtilsMessengerCreateInfo(debugCallbackInitInfo);
    instCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&vulkanDebugMessangerCreateInfo;
#endif

    if (vkCreateInstance(&instCreateInfo, nullptr, &s_vulkanState.instance) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan instance creation failed");
        return false;
    }

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    if (!InitVulkanDebugCallback(debugCallbackInitInfo)) {
        return false;
    }
#endif

    return true;
}


void VulkanApplication::TerminateVulkanInstance() noexcept
{
    vkDestroyInstance(s_vulkanState.instance, nullptr);
}


bool VulkanApplication::InitVulkanPhysicalDevice(const VulkanPhysDeviceInitInfo& initInfo) noexcept
{
    if (IsVulkanPhysicalDeviceInitialized()) {
        AM_LOG_WARN("Vulkan physical device is already initialized");
        return true;
    }

    AM_ASSERT_GRAPHICS_API(s_vulkanState.instance != VK_NULL_HANDLE, "Vulkan instance must be initialized before physical device initialization");

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(s_vulkanState.instance, &deviceCount, nullptr);

    if (deviceCount <= 0) {
        AM_ASSERT_GRAPHICS_API(false, "There are no any physical graphics devices that support Vulkan");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(s_vulkanState.instance, &deviceCount, devices.data());

    std::multimap<uint64_t, VkPhysicalDevice> suitableDevices;

    AM_LOG_GRAPHICS_API_INFO("Picking suitable physical device...");
    for (VkPhysicalDevice device : devices) {
        const uint64_t devicePriority = GetVulkanPhysicalDevicePriority(device, initInfo);
        
        if (devicePriority) {
            suitableDevices.insert(std::make_pair(devicePriority, device));
        }
    }

    if (suitableDevices.empty()) {
        AM_ASSERT_GRAPHICS_API(false, "There are no any Vulkan physical graphics devices that support required properties");
        return false;
    }

    for (const auto& it = suitableDevices.rbegin(); it != suitableDevices.rend(); std::next(it)) {
        if (FindRequiredVulkanQueueFamilies(it->second).IsComplete()) {
            s_vulkanState.device = it->second;
            break;
        }
    }

    if (s_vulkanState.device == VK_NULL_HANDLE) {
        return false;
    }

    return true;
}


bool VulkanApplication::InitVulkan() noexcept
{
    if (IsVulkanInitialized()) {
        AM_LOG_WARN("Vulkan is already initialized");
        return true;
    }

    const std::optional<nlohmann::json> vulkanConfigOpt = amjson::ParseJson(paths::AM_VULKAN_CONFIG_FILE_PATH);
    if (!vulkanConfigOpt.has_value()) {
        AM_ASSERT(false, "Failed to parse Vulkan config file");
        return false;
    }

    const nlohmann::json& vulkanConfigJson = vulkanConfigOpt.value();
    const nlohmann::json& instanceJson = vulkanConfigJson[JSON_VK_CONFIG_INSTANCE_FIELD_NAME];
    const nlohmann::json& physDeviceJson = vulkanConfigJson[JSON_VK_CONFIG_PHYS_DEVICE_FIELD_NAME];

    if (!InitVulkanInstance(ParseVulkanInstanceInitInfoJson(instanceJson))) {
        return false;
    }
    
    if (!InitVulkanPhysicalDevice(ParseVulkanPhysDeviceInitInfoJson(physDeviceJson))) {
        return false;
    }

    return true;
}


void VulkanApplication::TerminateVulkan() noexcept
{
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    TerminateVulkanDebugCallback();
#endif

    TerminateVulkanInstance();
}


bool VulkanApplication::IsVulkanInstanceInitialized() noexcept
{
    return s_vulkanState.instance != VK_NULL_HANDLE;
}


bool VulkanApplication::IsVulkanDebugCallbackInitialized() noexcept
{
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    return s_vulkanState.debugMessenger != VK_NULL_HANDLE;
#else
    return true;
#endif
}


bool VulkanApplication::IsVulkanPhysicalDeviceInitialized() noexcept
{
    return s_vulkanState.device != VK_NULL_HANDLE;
}


bool VulkanApplication::IsVulkanInitialized() noexcept
{
    return IsVulkanDebugCallbackInitialized() && IsVulkanDebugCallbackInitialized() && IsVulkanPhysicalDeviceInitialized();
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