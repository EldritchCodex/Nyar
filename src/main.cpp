// Nyar Renderer
//
// Baseline implementation following the Khronos Vulkan "Drawing a Triangle" tutorial.
// Serves as the working foundation before being iteratively refactored into a custom
// renderer API for the Nodens framework.
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ranges>
#include <vulkan/vk_platform.h>

import vulkan;
import std;

constexpr uint32_t WINDOW_WIDTH{800};
constexpr uint32_t WINDOW_HEIGHT{600};

const std::vector<char const*> validationLayers{"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers{false};
#else
constexpr bool enableValidationLayers{true};
#endif

class HelloTriangleApplication
{
public:
    void run()
    {
        initVulkan();
        setupDebugMessenger();
        pickPhysicalDevice();
        mainLoop();
        cleanup();
    }

private:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                  vk::DebugUtilsMessageTypeFlagsEXT type,
                  const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData)
    {
        std::println(
            stderr, "[validation layer][{}]: {}", to_string(type), pCallbackData->pMessage);

        return vk::False;
    }

    std::vector<const char*> getRequiredInstanceExtensionsNames()
    {
        // “Vulkan is a platform-agnostic API, which means that you need an extension to interface
        // with the window system. GLFW has a handy built-in function that returns the extension(s)
        // it needs to do that which we can pass to the struct”
        // - (“Khronos Vulkan Tutorial / Drawing a Triangle / Setup - Instance”)
        uint32_t glfwExtensionCount{0};
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    }

    void createInstance()
    {
        // DEFINE APPLICATION INFO /////////////////////////////////////////////////////////////////
        constexpr vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
                                              .applicationVersion = vk::makeApiVersion(0, 0, 0, 0),
                                              .pEngineName = "Nyar",
                                              .engineVersion = vk::makeApiVersion(0, 0, 0, 0),
                                              .apiVersion = vk::ApiVersion14};

        // EXTENSIONS //////////////////////////////////////////////////////////////////////////////
        // List available extensions
        auto availableExtensionsNames =
            context.enumerateInstanceExtensionProperties() |
            std::views::transform([](const auto& extentionProperty)
                                  { return std::string_view{extentionProperty.extensionName}; });
        std::println("Available extensions:");
        for (auto name : availableExtensionsNames)
        {
            std::println("\t{}", name);
        }

        // Check for required extensions
        auto requiredExtensionsNames = getRequiredInstanceExtensionsNames();
        auto unsupportedExtensionIt = std::ranges::find_if(
            requiredExtensionsNames,
            [&availableExtensionsNames](std::string_view requiredName)
            { return !std::ranges::contains(availableExtensionsNames, requiredName); });
        if (unsupportedExtensionIt != requiredExtensionsNames.end())
        {
            throw std::runtime_error{"Required GLFW extension not supported: " +
                                     std::string{*unsupportedExtensionIt}};
        }

        // LAYERS //////////////////////////////////////////////////////////////////////////////////
        // List available layers
        auto layersNames =
            context.enumerateInstanceLayerProperties() |
            std::views::transform([](const auto& layerProperty)
                                  { return std::string_view{layerProperty.layerName}; });
        std::println("Available layers:");
        for (auto name : layersNames)
        {
            std::println("\t{}", name);
        }

        // Check for required layers
        std::vector<char const*> requiredLayers{};
        if (enableValidationLayers)
        {
            requiredLayers.assign(validationLayers.begin(), validationLayers.end());
        }
        auto unsupportedLayerIt = std::ranges::find_if(
            requiredLayers,
            [&](std::string_view req) { return !std::ranges::contains(layersNames, req); });

        if (unsupportedLayerIt != requiredLayers.end())
        {
            throw std::runtime_error{"Required layer not supported " +
                                     std::string{*unsupportedLayerIt}};
        }

        // CREATE INSTANCE /////////////////////////////////////////////////////////////////////////
        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensionsNames.size()),
            .ppEnabledExtensionNames = requiredExtensionsNames.data()};

        instance = vk::raii::Instance{context, createInfo};
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags{
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError};
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags{
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral};
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags,
            .messageType = messageTypeFlags,
            .pfnUserCallback = &debugCallback};
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice)
    {
        bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

        auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        bool supportsGraphics = std::ranges::any_of(
            queueFamilies,
            [](const auto& qfp)
            { return static_cast<bool>(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

        std::vector<const char*> requiredDeviceExtension{vk::KHRDisplaySwapchainExtensionName};
        auto availableDeviceExtensionsNames =
            physicalDevice.enumerateDeviceExtensionProperties() |
            std::views::transform([](const auto& prop)
                                  { return std::string_view(prop.extensionName); });
        bool supportsAllRequiredExtensions = std::ranges::all_of(
            requiredDeviceExtension,
            [&availableDeviceExtensionsNames](std::string_view requiredDeviceExtension)
            {
                return std::ranges::contains(availableDeviceExtensionsNames,
                                             requiredDeviceExtension);
            });

        auto features =
            physicalDevice
                .template getFeatures2<vk::PhysicalDeviceFeatures2,
                                       vk::PhysicalDeviceVulkan11Features,
                                       vk::PhysicalDeviceVulkan13Features,
                                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures =
            features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
                .extendedDynamicState;

        return supportsVulkan1_3 &&
               supportsGraphics &&
               supportsAllRequiredExtensions &&
               supportsRequiredFeatures;
    }

    void pickPhysicalDevice()
    {
        auto availablePhysicalDevices = instance.enumeratePhysicalDevices();
        auto deviceIterator = std::ranges::find_if(
            availablePhysicalDevices, [&](const auto& dev) { return isDeviceSuitable(dev); });
        if (deviceIterator == availablePhysicalDevices.end())
        {
            throw std::runtime_error{"failed to find suitable GPU!"};
        }
        physicalDevice = *deviceIterator;
    }

    void initVulkan()
    {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);

        if (!glfwInit())
        {
            throw std::runtime_error{"GLFW Initialization failed."};
        }

        // GLFW was originally designed to work with OpenGL contexts,
        // so we need to tell it not to create one.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // For now, we don't deal with resizable windows.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);

        createInstance();
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
    GLFWwindow* window{nullptr};
    vk::raii::Context context{};
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
    vk::raii::PhysicalDevice physicalDevice{nullptr};
};

int main()
{
    try
    {
        HelloTriangleApplication app{};
        app.run();
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
