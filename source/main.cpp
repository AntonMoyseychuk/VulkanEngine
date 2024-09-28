#include "pch.h"

#include "application/vk_application.h"


int main(int argc, char* argv[])
{
    amInitLogSystem();
    
    if (!VulkanApplication::Init()) {
        AM_LOG_ERROR("Unexpected problems occurred during the initialization of the application. Application is terminated.\n");
        exit(-1);
    }
    
    VulkanApplication& instance = VulkanApplication::Instance();
    instance.Run();

    VulkanApplication::Terminate();

    AM_LOG_INFO("END");
    
    amTerminateLogSystem();

    return 0;
}