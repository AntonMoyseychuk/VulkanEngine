#include "application/vk_application.h"

#include "utils/debug/assertion.h"


int main(int argc, char* argv[])
{
    amInitLogSystem();
    
    if (!VulkanApplication::Init()) {
        exit(-1);
    }

    VulkanApplication& instance = VulkanApplication::Instance();
    instance.Run();

    VulkanApplication::Terminate();
    
    amTerminateLogSystem();

    return 0;
}