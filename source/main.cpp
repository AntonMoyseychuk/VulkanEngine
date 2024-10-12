#include "application/vk_application.h"

#include "utils/debug/assertion.h"


int main(int argc, char* argv[])
{
    amInitLogSystem();
    
    if (!VulkanApplication::Init()) {
        exit(-1);
    }

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Application initialized"));
    
    VulkanApplication& instance = VulkanApplication::Instance();
    instance.Run();

    VulkanApplication::Terminate();

    AM_LOG_INFO(AM_MAKE_COLORED_TEXT(AM_OUTPUT_COLOR_GREEN_ASCII_CODE, "Application terminated"));
    
    amTerminateLogSystem();

    return 0;
}