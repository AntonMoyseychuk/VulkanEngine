#include "pch.h"

#include "application/vk_application.h"

namespace fs = std::filesystem;


static VulkanAppInitInfo ParseVkAppInitInfoJson(const fs::path& pathToJson) noexcept
{
    enum {
        FILE_NOT_EXISTS = -1,
        FAILED_TO_OPEN_FILE = -2,
    };

    if (!fs::exists(pathToJson)) {
        AM_ERROR_MSG("[ERROR]: App config file \"%s\" doesn't exist\n", pathToJson.string().c_str());
        exit(FILE_NOT_EXISTS);
    }
    
    std::ifstream jsonFile(pathToJson);
    if (!jsonFile.is_open()) {
        AM_ERROR_MSG("[ERROR]: Failed to open file \"%s\"\n", pathToJson.string().c_str());
        exit(FAILED_TO_OPEN_FILE);
    }

    nlohmann::json data = nlohmann::json::parse(jsonFile);

    VulkanAppInitInfo appInitInfo = {};
    data["window"]["title"].get_to(appInitInfo.title);
    data["window"]["width"].get_to(appInitInfo.width);
    data["window"]["height"].get_to(appInitInfo.height);
    data["window"]["resizable"].get_to(appInitInfo.resizable);

    return appInitInfo;
}


int main(int argc, char* argv[])
{
    VulkanAppInitInfo appInitInfo = ParseVkAppInitInfoJson(paths::AM_PROJECT_CONFIG_FILE_PATH);
    
    if (!VulkanApplication::Init(appInitInfo)) {
        exit(-1);
    }
    
    VulkanApplication& instance = VulkanApplication::Instance();
    instance.Run();

    VulkanApplication::Terminate();

    return 0;
}