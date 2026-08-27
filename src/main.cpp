// Nyar Renderer
//
// Baseline implementation following the Khronos Vulkan "Drawing a Triangle" tutorial.
// Serves as the working foundation before being iteratively refactored into a custom
// renderer API for the Nodens framework.
#include <cstdlib>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

import vulkan;
import std;

class HelloTriangleApplication
{
public:
    void run()
    {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initVulkan()
    {
    }

    void mainLoop()
    {
    }

    void cleanup()
    {
    }
};

int main()
{
    try
    {
        HelloTriangleApplication app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
