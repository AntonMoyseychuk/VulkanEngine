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

// Application config specific field names
static constexpr const char* JSON_APP_CONFIG_WINDOW_FIELD_NAME = "window";
static constexpr const char* JSON_APP_CONFIG_WINDOW_TITLE_FIELD_NAME = "title";
static constexpr const char* JSON_APP_CONFIG_WINDOW_WIDTH_FIELD_NAME = "width";
static constexpr const char* JSON_APP_CONFIG_WINDOW_HEIGHT_FIELD_NAME = "height";
static constexpr const char* JSON_APP_CONFIG_WINDOW_RESIZABLE_FLAG_FIELD_NAME = "resizable";


static constexpr size_t MAX_VULKAN_EXTENSIONS_COUNT = 64u;
static constexpr size_t MAX_VULKAN_VALIDATION_LAYERS_COUNT = 64u;


// Potentialy unsafe, since lifetime of chPtrArr must be less of equal than strArr
static void MapArrayOfStringToArrayOfCharPtrs(const char** chPtrArr, size_t& inoutChPtrArrSize, const std::vector<std::string>& strArr) noexcept
{
    AM_ASSERT(strArr.size() <= inoutChPtrArrSize, "Target chPtrArr size is less than source strArr");

    inoutChPtrArrSize = strArr.size();
    for(size_t i = 0; i < inoutChPtrArrSize; ++i) {
        chPtrArr[i] = strArr[i].data();
    }
}


static const nlohmann::json& GetOsBuildTypeSpecificJsonObj(const nlohmann::json& base) noexcept
{
    const nlohmann::json& os = base[JSON_COMMON_CONFIG_OS_FIELD_NAME][JSON_COMMON_CONFIG_OS_NAME];
    return os[JSON_COMMON_CONFIG_BUILD_TYPE_FIELD_NAME][JSON_COMMON_CONFIG_BUILD_TYPE];
}


static std::optional<VulkanAppInitInfo> ParseVkAppInitInfoJson(const std::filesystem::path& pathToJson) noexcept
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


static inline std::string GetVulkanValidationLayerDesc(const VkLayerProperties& layer) noexcept
{
    return fmt::format("{} -> \033[33m{}\033[0m", layer.layerName, layer.description);
}

static inline std::string GetVulkanValidationLayerDesc(const std::string& layerDesc) noexcept
{
    return fmt::format("{}{}{}", "\033[33m", layerDesc, "\033[0m");
}

template<typename ValidationLayerT>
static std::string MakeVulkanValidationLayersListString(const std::vector<ValidationLayerT>& layers) noexcept
{
    std::string result = "";
        
    for (const ValidationLayerT& layer : layers) {
        result += fmt::format("{}{}{}", "\t- ", GetVulkanValidationLayerDesc(layer), ";\n");
    }

    return result;
}


static inline std::string GetVulkanExtensionDesc(const VkExtensionProperties& extension) noexcept
{
    return fmt::format("{}{}{}", "\033[33m", extension.extensionName, "\033[0m");
}

static inline std::string GetVulkanExtensionDesc(const std::string& extension) noexcept
{
    return fmt::format("{}{}{}", "\033[33m", extension, "\033[0m");
}

template<typename ExtensionT>
static std::string MakeVulkanExtensionsListString(const std::vector<ExtensionT>& extensions) noexcept
{
    std::string result = "";
        
    for (const ExtensionT& extension : extensions) {
        result += fmt::format("{}{}{}", "\t- ", GetVulkanExtensionDesc(extension), ";\n");
    }

    return result;
}


static std::vector<std::string> GetVulkanValidationLayers(const nlohmann::json& vkConfigJsonFile) noexcept
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
            AM_LOG_GRAPHICS_API_ERROR("Requested {} Vulkan extension in not found", requestedExtension);
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
    if (s_isAppInitialized) {
        AM_LOG_INFO("Application is already initialized");
        return true;
    }

    std::optional<VulkanAppInitInfo> appInitInfoOpt = ParseVkAppInitInfoJson(paths::AM_PROJECT_CONFIG_FILE_PATH);
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
        return false;
    }

    s_pAppInst = std::unique_ptr<VulkanApplication>(new VulkanApplication(appInitInfo));
    if (!s_pAppInst) {
        AM_ASSERT(false, "VulkanApplication allocation failed");
        return false;
    }

    if (!s_pAppInst->IsGlfwWindowCreated()) {
        return false;
    }

    s_isAppInitialized = true;
    return s_isAppInitialized;
}


void VulkanApplication::Terminate() noexcept
{
    s_pAppInst = nullptr;

    TerminateVulkan();
    glfwTerminate();
}


void VulkanApplication::Run() noexcept
{
    while(!glfwWindowShouldClose(m_glfwWindow)) {
        glfwPollEvents();
    }
}


bool VulkanApplication::InitVulkan(const char *appName) noexcept
{
    if (!appName) {
        AM_ASSERT(false, "appName is nullptr");
        return false;
    }

    const std::optional<nlohmann::json> jsonOpt = amjson::ParseJson(paths::AM_VULKAN_CONFIG_FILE_PATH);
    if (!jsonOpt.has_value()) {
        return false;
    }

    const nlohmann::json& json = jsonOpt.value();

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

    const std::vector<std::string> extensionNames = GetVulkanExtensions(json);
    const char* extensionNamesChar[MAX_VULKAN_EXTENSIONS_COUNT] = {};
    size_t extensionsCount = extensionNames.size();

    if (extensionsCount > 0) {
        MapArrayOfStringToArrayOfCharPtrs(extensionNamesChar, extensionsCount, extensionNames);
        instCreateInfo.ppEnabledLayerNames = extensionNamesChar;
    }

    instCreateInfo.enabledLayerCount = static_cast<uint32_t>(extensionsCount);
    
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    const std::vector<std::string> validationLayerNames = GetVulkanValidationLayers(json);
    const char* validationLayerNamesChar[MAX_VULKAN_VALIDATION_LAYERS_COUNT] = {};
    size_t validationLayersCount = validationLayerNames.size();
 
    if (validationLayersCount > 0) {
        MapArrayOfStringToArrayOfCharPtrs(validationLayerNamesChar, validationLayersCount, validationLayerNames);
        instCreateInfo.ppEnabledLayerNames = validationLayerNamesChar;
    }

    instCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersCount);
#endif

    if (vkCreateInstance(&instCreateInfo, nullptr, &s_vulkanInst) != VK_SUCCESS) {
        AM_ASSERT_GRAPHICS_API(false, "Vulkan instance creation failed");
    }

    return true;
}


void VulkanApplication::TerminateVulkan() noexcept
{
    vkDestroyInstance(s_vulkanInst, nullptr);
}


VulkanApplication::VulkanApplication(const VulkanAppInitInfo &appInitInfo)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, appInitInfo.resizable ? GLFW_TRUE : GLFW_FALSE);

    m_glfwWindow = glfwCreateWindow((int)appInitInfo.width, (int)appInitInfo.height, appInitInfo.title.c_str(), nullptr, nullptr);
    AM_ASSERT_WINDOW(m_glfwWindow, "GLFW window creation failed");
}


bool VulkanApplication::IsGlfwWindowCreated() const noexcept
{
    return m_glfwWindow != nullptr;
}


VulkanApplication::~VulkanApplication()
{
    glfwDestroyWindow(m_glfwWindow);
}