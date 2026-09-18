module;
#include <GLFW/glfw3.h>

module NyarLayer;
import nodens;

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

void NyarLayer::OnDetach()
{
    cleanup();
}

void NyarLayer::OnUpdate(Nodens::TimeStep ts)
{
    // Temporary workaround handling close window event.
    // We are using Nodens headless and managing the windowing ourselves for now, so we need to check for the window
    // close event.
    if (glfwWindowShouldClose(window))
    {
        Nodens::RoutedInputEvent event{.Event = Nodens::InputEvents::WindowClose{}};
        Nodens::Application::Get().OnInputEvent(event);
        return;
    }

    glfwPollEvents();
    drawFrame();
}
