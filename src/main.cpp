// Nyar Renderer
//
// Baseline implementation following the Khronos Vulkan "Drawing a Triangle" tutorial.
// Serves as the working foundation before being iteratively refactored into a custom
// renderer API for the Nodens framework.
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <print>
#include <stdexcept>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

import vulkan;
import std;

constexpr uint32_t WINDOW_WIDTH = 800;
constexpr uint32_t WINDOW_HEIGHT = 600;

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
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);

        if (!glfwInit())
        {
            throw std::runtime_error("GLFW Initialization failed.");
        }

        // GLFW was originally designed to work with OpenGL contexts,
        // so we need to tell it not to create one.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // For now, we don't deal with resizable windows.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

private:
    GLFWwindow* window = nullptr;
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
        std::println(stderr, "Error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
