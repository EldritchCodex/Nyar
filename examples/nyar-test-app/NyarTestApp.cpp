#include <cassert> // Needed for `assert`
#include <cstdio>  // Needed for `stderr`
#include <cstdlib> // Needed for `EXIT_FAILURE` and `EXIT_SUCCESS`

import NyarLayer;
import vulkan;
import nodens;

// -----------------------------------------------------------------------------
// Step 3: Define the Application class, the root object in charge of the
// application, managing the window, layers, and the main event loop.
// -----------------------------------------------------------------------------

class NyarTestApp : public Nodens::Application
{
public:
    static inline const Nodens::ApplicationSpecification appSpecifications = {
        .Name = "Nyar Vulkan Development",
        .WindowWidth = 800,
        .WindowHeight = 600,
        .EnableGUI = false,
        .IsHeadless = true,
    };

    NyarTestApp() : Application(appSpecifications)
    {
        PushLayer(new NyarLayer{});
    }

    ~NyarTestApp() = default;
};

// -----------------------------------------------------------------------------
// Step 4: Application entry point.
// -----------------------------------------------------------------------------

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
