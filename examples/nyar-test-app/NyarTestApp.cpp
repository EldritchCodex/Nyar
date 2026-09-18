/// @file NyarTestApp.cpp
/// @brief Minimal Nodens application hosting the Vulkan tutorial layer.
/// @details This executable provides the Hello Triangle integration baseline.
///          Future renderer work moves behind Nyar's public module while this
///          application remains a small validation program.
/// @ingroup Examples

#include <cassert>
#include <cstdio>
#include <cstdlib>

import NyarLayer;
import vulkan;
import nodens;

/// @brief Root Nodens application for the Vulkan development example.
/// @details Nodens creates the Vulkan-backed GLFW window. Nyar borrows its
///          instance and surface for rendering.
/// @ingroup Examples
class NyarTestApp : public Nodens::Application
{
public:
    /// Application configuration passed to the Nodens base class.
    static inline const Nodens::FApplicationSpecification appSpecifications = {
        .Name = "Nyar Vulkan Development",
        .WindowWidth = 800,
        .WindowHeight = 600,
        .EnableGUI = false,
        .IsHeadless = false,
        .GraphicsAPI = Nodens::EGraphicsAPI::Vulkan,
    };

    /// @brief Constructs application and installs the Vulkan layer.
    NyarTestApp() : Application(appSpecifications)
    {
        PushLayer(new NyarLayer{});
    }

    ~NyarTestApp() = default;
};

/// @brief Starts Nodens and reports process-level failures.
int main()
{
    Nodens::InitializeLoggers();
    try
    {
        auto app = NyarTestApp{};
        app.Run();
    }
    catch (const vk::SystemError& err)
    {
        std::println(stderr, "Vulkan Error {}", err.what());
        return EXIT_FAILURE;
    }
    catch (const std::exception& err)
    {
        std::println(stderr, "Error: {}", err.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
