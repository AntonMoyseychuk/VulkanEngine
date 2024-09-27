#include "pch.h"

#include "application/vk_application.h"


int main(int argc, char* argv[])
{
#if defined(AM_LOGGING_ENABLED)
    Logger::Init();
#endif
    
    if (!VulkanApplication::Init()) {
        exit(-1);
    }
    
    VulkanApplication& instance = VulkanApplication::Instance();
    instance.Run();

    VulkanApplication::Terminate();

#if defined(AM_LOGGING_ENABLED)
    Logger::Terminate();
#endif

    return 0;
}