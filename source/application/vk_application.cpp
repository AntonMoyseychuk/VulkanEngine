#include "../pch.h"
#include "vk_application.h"


static const char* VULKAN_ENGINE_NAME = "Vulkan Engine";


#if defined(AM_LOGGING_ENABLED)
inline const char* GetExtensionName(const char* extension) noexcept
{
    return extension;
}

inline const char* GetExtensionName(const std::string& extension) noexcept
{
    return extension.c_str();
}

inline const char* GetExtensionName(const VkExtensionProperties& extension) noexcept
{
    return extension.extensionName;
}


template<typename ExtT, typename... Args>
static void LogVulkanExtensions(const ExtT* extensions, size_t extensionsCount, const char* format, Args&&... args)
{
    AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stdout, format, std::forward<Args>(args)...);

    if (!extensions) {
        return;
    }
    
    for (size_t i = 0; i < extensionsCount; ++i) {
        AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stdout, "\t- %s;\n", GetExtensionName(extensions[i]));
    }
}


template<typename... Args>
static void LogVulkanValidationLayers(const VkLayerProperties* layers, size_t layersCount, const char* format, Args&&... args)
{
    AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stdout, format, std::forward<Args>(args)...);

    if (!layers) {
        return;
    }
    
    for (size_t i = 0; i < layersCount; ++i) {
        AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stdout, "\t- %s", layers[i].layerName);
        AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_YELLOW, stdout, " -> %s;\n", layers[i].description);
    }
}

static void LogVulkanValidationLayers(const std::string* layers, size_t layersCount, const char* format)
{
    AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stdout, format);

    if (!layers) {
        return;
    }
    
    for (size_t i = 0; i < layersCount; ++i) {
        AM_PRINT_FORMATED_COLORED_TEXT(COLOR_CODE_WHITE, stdout, "\t- %s;\n", layers[i].c_str());
    }
}
#else
    #define LogVulkanExtensions(extensions, extensionsCount, format, ...)
    #define LogVulkanValidationLayers(layers, layersCount, format, ...)
#endif


struct GetJsonResult
{
    enum GetJsonResultErrorReason {
        GetJsonResultErrorReason_NO_ERROR,
        GetJsonResultErrorReason_FILE_NOT_EXISTS,
        GetJsonResultErrorReason_FAILED_TO_OPEN_FILE
    };

    std::optional<nlohmann::json> json;
    GetJsonResultErrorReason errorCode;
};


static GetJsonResult GetJson(const std::filesystem::path& pathToJson) noexcept
{
    if (!std::filesystem::exists(pathToJson)) {
        return {{}, GetJsonResult::GetJsonResultErrorReason_FILE_NOT_EXISTS};
    }
    
    std::ifstream jsonFile(pathToJson);
    if (!jsonFile.is_open()) {
        return {{}, GetJsonResult::GetJsonResultErrorReason_FAILED_TO_OPEN_FILE};
    }

    return {nlohmann::json::parse(jsonFile), GetJsonResult::GetJsonResultErrorReason_NO_ERROR};
}


static std::vector<std::string> ParseJsonArray(const std::filesystem::path& pathToJson, const char* fieldName)
{
    const GetJsonResult jsonRes = GetJson(pathToJson);

    switch (jsonRes.errorCode) {
        case GetJsonResult::GetJsonResultErrorReason_FILE_NOT_EXISTS:
            AM_WARNING_MSG("\n[WARNING]: File \"%s\" doesn't exist\n", pathToJson.string().c_str());
            break;

        case GetJsonResult::GetJsonResultErrorReason_FAILED_TO_OPEN_FILE:
            AM_WARNING_MSG("\n[WARNING]: Failed to open file \"%s\"\n", pathToJson.string().c_str());
            break;

        case GetJsonResult::GetJsonResultErrorReason_NO_ERROR:
            break;
    }

    if (jsonRes.errorCode != GetJsonResult::GetJsonResultErrorReason_NO_ERROR) {
        return {};
    }

    std::vector<std::string> resultItems;

    const auto& layers = jsonRes.json.value()[fieldName];
    resultItems.reserve(layers.size());

    for (const auto& layer : layers.items()) {
        resultItems.emplace_back(layer.value().get<std::string>());
    }

    return resultItems;
}


#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
static std::vector<std::string> GetVkValidationLayers() noexcept
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    LogVulkanValidationLayers(availableLayers.data(), layerCount, "\n[INFO]: Vulkan supported validation layers:\n");

    static const std::vector<std::string> validationLayers = ParseJsonArray(paths::AM_VULKAN_CONFIG_FILE_PATH, "validation_layers");

    for (const std::string& requiredLayer : validationLayers) {
        bool layerFound = false;

        for (const VkLayerProperties& availableLayer : availableLayers) {
            if (strcmp(requiredLayer.data(), availableLayer.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return {};
        }
    }

    return validationLayers;
}
#endif


static std::vector<std::string> GetVkExtensions() noexcept
{
    uint32_t supportedVulkanExtCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedVulkanExtCount, nullptr);

    std::vector<VkExtensionProperties> supportedVkExtensions(supportedVulkanExtCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &supportedVulkanExtCount, supportedVkExtensions.data());
    LogVulkanExtensions(supportedVkExtensions.data(), supportedVulkanExtCount, "[INFO]: Vulkan supported extensions:\n");
    
    std::vector<std::string> extensions = ParseJsonArray(paths::AM_VULKAN_CONFIG_FILE_PATH, "extensions");

    for (const std::string& requiredExt : extensions) {
        bool extFound = false;

        for (const VkExtensionProperties& availableExt : supportedVkExtensions) {
            if (strcmp(requiredExt.data(), availableExt.extensionName) == 0) {
                extFound = true;
                break;
            }
        }

        if (!extFound) {
            AM_WARNING_MSG("[WARNING]: Vulkan extension %s was not found!\n", requiredExt.c_str());
        }
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    LogVulkanExtensions(glfwExtensions, glfwExtensionCount, "\n[INFO]: Vulkan extensions required by GLFW:\n\r");

    for (size_t i = 0; i < glfwExtensionCount; ++i) {
        const char* glfwExt = glfwExtensions[i];

        bool isExtFound = false;
        for (const std::string& ext : extensions) {
            isExtFound = strcmp(ext.c_str(), glfwExt) == 0;

            if (isExtFound) {
                break;
            }
        }

        if (!isExtFound) {
            extensions.emplace_back(glfwExt);
        }
    }

    return extensions;
}


VulkanApplication& VulkanApplication::Instance() noexcept
{
    AM_ASSERT(s_pAppInst, "Application is not initialized, call Application::Init(...) first");
    
    return *s_pAppInst;
}


bool VulkanApplication::Init(const VulkanAppInitInfo &appInitInfo) noexcept
{
    if (s_isAppInitialized) {
        AM_WARNING_MSG("[APP WARNING]: Application is already initialized\n");
        return true;
    }

#if defined(AM_LOGGING_ENABLED)
    glfwSetErrorCallback([](int errorCode, const char* description) -> void {
        AM_ERROR_MSG("-- [GLFW ERROR]: %s (code: %d)\n", description, errorCode);
    });
#endif
    
    if (glfwInit() != GLFW_TRUE) {
        AM_ASSERT(false, "GLFW initialization failed");
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
        AM_ASSERT(appName, "appName is nullptr");
        return false;
    }

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

    const std::vector<std::string> extensionNames = GetVkExtensions();
    const char** extensionNamesChar = nullptr;

    LogVulkanExtensions(extensionNames.data(), extensionNames.size(), "\n[INFO]: Final Vulkan included extensions:\n");
    
    if (extensionNames.empty()) {
        AM_LOG_WARNING("\n[WARNING]: There are no required extensions or they are unavailable\n");

        instCreateInfo.enabledExtensionCount = 0;
        instCreateInfo.ppEnabledExtensionNames = nullptr;
    } else {
        extensionNamesChar = (const char**)_alloca(sizeof(const char**) * extensionNames.size());
        for (size_t layer = 0; layer < extensionNames.size(); ++layer) {
            extensionNamesChar[layer] = extensionNames[layer].data();
        }

        instCreateInfo.enabledLayerCount = static_cast<uint32_t>(extensionNames.size());
        instCreateInfo.ppEnabledLayerNames = extensionNamesChar;
    }
    
#if defined(AM_VK_VALIDATION_LAYERS_ENABLED)
    const std::vector<std::string> validationLayerNames = GetVkValidationLayers();
    const char** validationLayerNamesChar = nullptr;

    LogVulkanValidationLayers(validationLayerNames.data(), validationLayerNames.size(), "\n[INFO]: Final Vulkan included validation layers:\n");
    
    if (validationLayerNames.empty()) {
        instCreateInfo.enabledLayerCount = 0;
        instCreateInfo.ppEnabledLayerNames = nullptr;
    } else {
        validationLayerNamesChar = (const char**)_alloca(sizeof(const char**) * validationLayerNames.size());
        for (size_t layer = 0; layer < validationLayerNames.size(); ++layer) {
            validationLayerNamesChar[layer] = validationLayerNames[layer].data();
        }

        instCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayerNames.size());
        instCreateInfo.ppEnabledLayerNames = validationLayerNamesChar;
    }
#else
    instCreateInfo.enabledLayerCount = 0;
    instCreateInfo.ppEnabledLayerNames = nullptr;
#endif

    if (vkCreateInstance(&instCreateInfo, nullptr, &s_vulkanInst) != VK_SUCCESS) {
        AM_ASSERT(false, "Vulkan instance creation failed");
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
    AM_ASSERT(m_glfwWindow, "GLFW window creation failed");
}


bool VulkanApplication::IsGlfwWindowCreated() const noexcept
{
    return m_glfwWindow != nullptr;
}


VulkanApplication::~VulkanApplication()
{
    glfwDestroyWindow(m_glfwWindow);
}