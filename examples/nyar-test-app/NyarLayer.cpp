/// @file NyarLayer.cpp
/// @brief Connects the Vulkan tutorial layer to Nodens.
/// @details Nodens owns the GLFW window, instance, and surface. The layer owns
///          Vulkan rendering resources built on those borrowed objects.
/// @ingroup Examples

module;
#include <GLFW/glfw3.h>

module NyarLayer;
import nodens;

/// @brief Attaches to Nodens Vulkan state and initializes rendering resources.
void NyarLayer::OnAttach()
{
    attachToNodensWindow();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

/// @brief Releases Vulkan rendering resources after layer removal.
void NyarLayer::OnDetach()
{
    cleanup();
}

/// @brief Submits one rendered frame.
void NyarLayer::OnUpdate(Nodens::TimeStep ts)
{
    drawFrame();
}
