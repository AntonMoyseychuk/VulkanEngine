#include "../pch.h"
#include "vk_application.h"


static constexpr const char* VULKAN_ENGINE_NAME = "Vulkan Engine";

static constexpr const char* JSON_COMMON_CONFIG_OS_FIELD_NAME = "os";
static constexpr const char* JSON_COMMON_CONFIG_BUILD_TYPE_FIELD_NAME = "build_type";

#if defined(AM_OS_WINDOWS)
    static constexpr const char* JSON_COMMON_CONFIG_OS_NAME = "win32";
#else
    #error Currently, only Windows is supported
#endif

#if defined(AM_DEBUG)
    static constexpr const char* JSON_COMMON_CONFIG_BUILD_TYPE = "debug";
#else
    static constexpr const char* JSON_COMMON_CONFIG_BUILD_TYPE = "release";
#endif


// Vulkan config specific field names
static constexpr const char* JSON_VK_CONFIG_VALIDATION_LAYERS_FIELD_NAME = "validation_layers";
static constexpr const char* JSON_VK_CONFIG_EXTENSIONS_FIELD_NAME = "extensions";

#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_FIELD_NAME = "callback";
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_SEVERITY_FIELD_NAME = "severity";
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPES_FIELD_NAME = "message_types";

    static constexpr const char* JSON_VK_CONFIG_CALLBACK_SEVERITY_ALL = "all";
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_SEVERITY_ERROR = "error";
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_SEVERITY_WARNING = "warning";
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_SEVERITY_INFO = "info";
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_SEVERITY_VERBOSE = "verbose";

    static constexpr const char* JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_ALL = "all";
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_GENERAL = "general";
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_VALIDATION = "validation";
    static constexpr const char* JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_PERFORMANCE = "performance";
#endif

// Application config specific field names
static constexpr const char* JSON_APP_CONFIG_WINDOW_FIELD_NAME = "window";
static constexpr const char* JSON_APP_CONFIG_WINDOW_TITLE_FIELD_NAME = "title";
static constexpr const char* JSON_APP_CONFIG_WINDOW_WIDTH_FIELD_NAME = "width";
static constexpr const char* JSON_APP_CONFIG_WINDOW_HEIGHT_FIELD_NAME = "height";
static constexpr const char* JSON_APP_CONFIG_WINDOW_RESIZABLE_FLAG_FIELD_NAME = "resizable";


AM_MAYBE_UNUSED static constexpr size_t MAX_VULKAN_EXTENSIONS_COUNT = 64u;
AM_MAYBE_UNUSED static constexpr size_t MAX_VULKAN_VALIDATION_LAYERS_COUNT = 64u;


#if defined(AM_LOGGING_ENABLED)
struct VulkanDebugCallbackInitInfo
{
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity;
    VkDebugUtilsMessageTypeFlagsEXT messageType;
    PFN_vkDebugUtilsMessengerCallbackEXT pCallback;
};
#endif


AM_MAYBE_UNUSED static const nlohmann::json& GetOsBuildTypeSpecificJsonObj(const nlohmann::json& base) noexcept
{
    const nlohmann::json& os = base[JSON_COMMON_CONFIG_OS_FIELD_NAME][JSON_COMMON_CONFIG_OS_NAME];
    return os[JSON_COMMON_CONFIG_BUILD_TYPE_FIELD_NAME][JSON_COMMON_CONFIG_BUILD_TYPE];
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

        if (severity == JSON_VK_CONFIG_CALLBACK_SEVERITY_ALL) {
            severityBits = SEVERITY_ALL;
        } else if (severity == JSON_VK_CONFIG_CALLBACK_SEVERITY_ERROR) {
            severityBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        } else if (severity == JSON_VK_CONFIG_CALLBACK_SEVERITY_WARNING) {
            severityBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        } else if (severity == JSON_VK_CONFIG_CALLBACK_SEVERITY_INFO) {
            severityBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        } else if (severity == JSON_VK_CONFIG_CALLBACK_SEVERITY_VERBOSE) {
            severityBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        } else {
            AM_ASSERT(false, "Invalid Vulkan callback message severity JSON tokken {}. Must be one of:\n\t- {};\n\t- {};\n\t- {};\n\t- {};\n\t- {};", 
                severity, 
                JSON_VK_CONFIG_CALLBACK_SEVERITY_ALL, 
                JSON_VK_CONFIG_CALLBACK_SEVERITY_ERROR, 
                JSON_VK_CONFIG_CALLBACK_SEVERITY_WARNING,
                JSON_VK_CONFIG_CALLBACK_SEVERITY_INFO,
                JSON_VK_CONFIG_CALLBACK_SEVERITY_VERBOSE);
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

        if (type == JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_ALL) {
            typeBits = TYPE_ALL;
        } else if (type == JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_GENERAL) {
            typeBits = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        } else if (type == JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_VALIDATION) {
            typeBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        } else if (type == JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_PERFORMANCE) {
            typeBits = (uint32_t)VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        } else {
            AM_ASSERT(false, "Invalid Vulkan callback message type JSON tokken {}. Must be one of:\n\t- {};\n\t- {};\n\t- {};\n\t- {};", 
                type, 
                JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_ALL, 
                JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_GENERAL, 
                JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_VALIDATION,
                JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPE_PERFORMANCE);
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


static VulkanDebugCallbackInitInfo ParseVulkankAppInitInfoJson(const nlohmann::json& vkConfigJsonFile) noexcept
{
    VulkanDebugCallbackInitInfo result = {};
    result.pCallback = VulkanDebugCallback;

    const nlohmann::json& build = GetOsBuildTypeSpecificJsonObj(vkConfigJsonFile);
    const nlohmann::json& callback = build[JSON_VK_CONFIG_CALLBACK_FIELD_NAME];

    const nlohmann::json& severity = callback[JSON_VK_CONFIG_CALLBACK_SEVERITY_FIELD_NAME];
    std::vector<std::string> severityTokens = amjson::ParseArrayJson<std::string>(severity);
    result.messageSeverity = GetVkDebugUtilsMessageSeverityFlagBits(severityTokens);

    const nlohmann::json& messageTypes = callback[JSON_VK_CONFIG_CALLBACK_MESSAGE_TYPES_FIELD_NAME];
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


static std::optional<VulkanAppInitInfo> ParseVulkanAppInitInfoJson(const std::filesystem::path& pathToJson) noexcept
{
    std::optional<nlohmann::json> jsonOpt = amjson::ParseJson(pathToJson);
    if (!jsonOpt.has_value()) {
        return {};
    }

    const nlohmann::json& build = GetOsBuildTypeSpecificJsonObj(jsonOpt.value());
    const nlohmann::json& window = build[JSON_APP_CONFIG_WINDOW_FIELD_NAME];

    VulkanAppInitInfo appInitInfo = {};
    window[JSON_APP_CONFIG_WINDOW_TITLE_FIELD_NAME].get_to(appInitInfo.title);
    window[JSON_APP_CONFIG_WINDOW_WIDTH_FIELD_NAME].get_to(appInitInfo.width);
    window[JSON_APP_CONFIG_WINDOW_HEIGHT_FIELD_NAME].get_to(appInitInfo.height);
    window[JSON_APP_CONFIG_WINDOW_RESIZABLE_FLAG_FIELD_NAME].get_to(appInitInfo.resizable);

    return appInitInfo;
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


AM_MAYBE_UNUSED static std::vector<std::string> GetVulkanValidationLayers(const nlohmann::json& vkConfigJsonFile) noexcept
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    AM_LOG_GRAPHICS_API_INFO("Available Vulkan validation layers:\n{}", MakeVulkanValidationLayersListString(availableLayers));

    const nlohmann::json& build = GetOsBuildTypeSpecificJsonObj(vkConfigJsonFile);
    const nlohmann::json& layers = build[JSON_VK_CONFIG_VALIDATION_LAYERS_FIELD_NAME];
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


static std::vector<std::string> GetVulkanExtensions(const nlohmann::json& vkConfigJsonFile) noexcept
{
    uint32_t availableVulkanExtCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableVulkanExtCount, nullptr);

    std::vector<VkExtensionProperties> availableVkExtensions(availableVulkanExtCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableVulkanExtCount, availableVkExtensions.data());

    AM_LOG_GRAPHICS_API_INFO("Available Vulkan extensions:\n{}", MakeVulkanExtensionsListString(availableVkExtensions));

    const nlohmann::json& build = GetOsBuildTypeSpecificJsonObj(vkConfigJsonFile);
    const nlohmann::json& extensions = build[JSON_VK_CONFIG_EXTENSIONS_FIELD_NAME];
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

    if (!InitVulkan(appInitInfo.title.c_str())) {
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


bool VulkanApplication::InitVulkanPhysicalDevice() noexcept
{
    AM_ASSERT_GRAPHICS_API(true, "Vulkan physical device initialization failed");
    return true;
}


bool VulkanApplication::InitVulkan(const char *appName) noexcept
{
    if (!appName) {
        appName = "Default Vulkan Application";
        AM_LOG_WARN("Vulkan appName is nullptr. Using default name {}", appName);
    }

    const std::optional<nlohmann::json> jsonOpt = amjson::ParseJson(paths::AM_VULKAN_CONFIG_FILE_PATH);
    if (!jsonOpt.has_value()) {
        AM_ASSERT(false, "Failed to parse Vulkan config file");
        return false;
    }

    const nlohmann::json& base = jsonOpt.value();

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = VULKAN_ENGINE_NAME;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instCreateInfo = {};
    instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.pApplicationInfo = &appInfo;
    instCreateInfo.enabledLayerCount = 0;
    instCreateInfo.ppEnabledLayerNames = nullptr;

    const std::vector<std::string> extensionNames = GetVulkanExtensions(base);
    const char* extensionNamesChar[MAX_VULKAN_EXTENSIONS_COUNT] = {};
    size_t extensionsCount = extensionNames.size();

    if (extensionsCount > 0) {
        MapArrayOfStringToArrayOfCharPtrs(extensionNamesChar, extensionsCount, extensionNames);
        instCreateInfo.ppEnabledExtensionNames = extensionNamesChar;
    }

    instCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsCount);
    
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    const std::vector<std::string> validationLayerNames = GetVulkanValidationLayers(base);
    const char* validationLayerNamesChar[MAX_VULKAN_VALIDATION_LAYERS_COUNT] = {};
    size_t validationLayersCount = validationLayerNames.size();
 
    if (validationLayersCount > 0) {
        MapArrayOfStringToArrayOfCharPtrs(validationLayerNamesChar, validationLayersCount, validationLayerNames);
        instCreateInfo.ppEnabledLayerNames = validationLayerNamesChar;
    }

    instCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersCount);

    VulkanDebugCallbackInitInfo debugInitInfo = ParseVulkankAppInitInfoJson(base);
    VkDebugUtilsMessengerCreateInfoEXT vulkanDebugMessangerCreateInfo = GetVkDebugUtilsMessengerCreateInfo(debugInitInfo);
    instCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&vulkanDebugMessangerCreateInfo;
#endif

    if (vkCreateInstance(&instCreateInfo, nullptr, &s_vulkanState.instance) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan instance creation failed");
        return false;
    }
    
    if (!InitVulkanPhysicalDevice()) {
        return false;
    }
    
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    if (!InitVulkanDebugCallback(debugInitInfo)) {
        return false;
    }
#endif

    return true;
}


void VulkanApplication::TerminateVulkan() noexcept
{
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    TerminateVulkanDebugCallback();
#endif

    vkDestroyInstance(s_vulkanState.instance, nullptr);
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