#include "application/vk_application.h"


int main(int argc, char* argv[])
{   
    if (!VulkanApplication::Init()) {
        exit(-1);
    }

    VulkanApplication& instance = VulkanApplication::Instance();
    instance.Run();

    VulkanApplication::Terminate();

    return 0;
}