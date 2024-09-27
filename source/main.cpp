#include "pch.h"

#include "application/vk_application.h"


int main(int argc, char* argv[])
{
#if defined(AM_LOGGING_ENABLED)
    if (!Logger::Init()) {
        AM_LOG_WARN("Unexpected problems occurred during the initialization of the log system. Using default logger.");
    }
#endif
    
    if (!VulkanApplication::Init()) {
        AM_LOG_ERROR("Unexpected problems occurred during the initialization of the application. Application is terminated.");
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