/// @file NyarLayer.cpp
/// @brief Connects the Vulkan tutorial layer to Nodens.
/// @details GLFW remains layer-owned while Nodens runs this example headless.
///          The layer forwards window-close state into Nodens until
///          Vulkan-backed windows become part of Nodens' lifecycle.
/// @ingroup Examples

module;
#include <GLFW/glfw3.h>

module NyarLayer;
import nodens;

/// @brief Initializes window and Vulkan resources in dependency order.
void NyarLayer::OnAttach()
{
    initWindow();
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

/// @brief Releases Vulkan and GLFW resources after layer removal.
void NyarLayer::OnDetach()
{
    cleanup();
}

/// @brief Polls GLFW, forwards close requests, and submits one frame.
void NyarLayer::OnUpdate(Nodens::TimeStep ts)
{
    // Nodens is headless, so translate GLFW close state manually.
    if (glfwWindowShouldClose(window))
    {
        Nodens::RoutedInputEvent event{.Event = Nodens::InputEvents::WindowClose{}};
        Nodens::Application::Get().OnInputEvent(event);
        return;
    }

    glfwPollEvents();
    drawFrame();
}
