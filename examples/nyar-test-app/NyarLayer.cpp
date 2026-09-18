/// @file NyarLayer.cpp
/// @brief Connects the Vulkan tutorial layer to Nodens.
/// @details Nodens owns the GLFW window and Vulkan platform resources. The layer
///          borrows those resources and owns rendering resources built on them.
/// @ingroup Examples

module;

module NyarLayer;
import nodens;

/// @brief Attaches to Nodens Vulkan state and initializes rendering resources.
void NyarLayer::OnAttach()
{
    attachToNodensWindow();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
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
