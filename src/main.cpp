#include <iostream>
#include <stdexcept>

#include "Vulkan/VulkanApplication.h"

int main() {
    VulkanApplication app;

    try {
        app.mainLoop();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}