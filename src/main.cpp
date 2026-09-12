// Nyar Renderer
//
// Baseline implementation following the Khronos Vulkan "Drawing a Triangle"
// tutorial. Serves as the working foundation before being iteratively
// refactored into a custom renderer API for the Nodens framework.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cassert> // Needed for `assert`
#include <cstdio>  // Needed for `stderr`
#include <cstdlib> // Needed for `EXIT_FAILURE` and `EXIT_SUCCESS`
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
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();

        mainLoop();
        cleanup();
    }

private:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                          void* pUserData)
    {
        std::println(stderr, "[validation layer][{}]: {}", to_string(type), pCallbackData->pMessage);

        return vk::False;
    }

    std::vector<const char*> getRequiredInstanceExtensionsNames()
    {
        // “Vulkan is a platform-agnostic API, which means that you need an extension to interface with the window
        // system. GLFW has a handy built-in function that returns the extension(s) it needs to do that which we can
        // pass to the struct”
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
        // DEFINE APPLICATION INFO /////////////////////////////////////////////////////////////////////////////////////
        constexpr vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
                                              .applicationVersion = vk::makeApiVersion(0, 0, 0, 0),
                                              .pEngineName = "Nyar",
                                              .engineVersion = vk::makeApiVersion(0, 0, 0, 0),
                                              .apiVersion = vk::ApiVersion14};

        // EXTENSIONS //////////////////////////////////////////////////////////////////////////////////////////////////
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
        auto unsupportedExtensionIt =
            std::ranges::find_if(requiredExtensionsNames,
                                 [&availableExtensionsNames](std::string_view requiredName)
                                 { return !std::ranges::contains(availableExtensionsNames, requiredName); });
        if (unsupportedExtensionIt != requiredExtensionsNames.end())
        {
            throw std::runtime_error{"Required GLFW extension not supported: " + std::string{*unsupportedExtensionIt}};
        }

        // LAYERS //////////////////////////////////////////////////////////////////////////////////////////////////////
        // List available layers
        auto layersNames =
            context.enumerateInstanceLayerProperties() |
            std::views::transform([](const auto& layerProperty) { return std::string_view{layerProperty.layerName}; });
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
            requiredLayers, [&](std::string_view req) { return !std::ranges::contains(layersNames, req); });

        if (unsupportedLayerIt != requiredLayers.end())
        {
            throw std::runtime_error{"Required layer not supported " + std::string{*unsupportedLayerIt}};
        }

        // CREATE INSTANCE /////////////////////////////////////////////////////////////////////////////////////////////
        vk::InstanceCreateInfo createInfo{.pApplicationInfo = &appInfo,
                                          .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
                                          .ppEnabledLayerNames = requiredLayers.data(),
                                          .enabledExtensionCount =
                                              static_cast<uint32_t>(requiredExtensionsNames.size()),
                                          .ppEnabledExtensionNames = requiredExtensionsNames.data()};

        instance = vk::raii::Instance{context, createInfo};
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags{vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError};
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags{vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral};
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags, .messageType = messageTypeFlags, .pfnUserCallback = &debugCallback};
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    void createSurface()
    {
        VkSurfaceKHR rawSurface{VK_NULL_HANDLE};
        if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != 0)
        {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::raii::SurfaceKHR(instance, rawSurface);
    }

    bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice)
    {
        bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

        auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        bool supportsGraphics = std::ranges::any_of(
            queueFamilies,
            [](const auto& qfp) { return static_cast<bool>(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

        std::vector<const char*> requiredDeviceExtensions{vk::KHRSwapchainExtensionName};
        auto availableDeviceExtensionsNames =
            physicalDevice.enumerateDeviceExtensionProperties() |
            std::views::transform([](const auto& prop) { return std::string_view(prop.extensionName); });
        bool supportsAllRequiredExtensions = std::ranges::all_of(
            requiredDeviceExtensions,
            [&availableDeviceExtensionsNames](std::string_view requiredDeviceExtensions)
            { return std::ranges::contains(availableDeviceExtensionsNames, requiredDeviceExtensions); });

        auto features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
                                                             vk::PhysicalDeviceVulkan11Features,
                                                             vk::PhysicalDeviceVulkan13Features,
                                                             vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures =
            features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
    }

    void pickPhysicalDevice()
    {
        auto availablePhysicalDevices = instance.enumeratePhysicalDevices();
        auto deviceIterator =
            std::ranges::find_if(availablePhysicalDevices, [&](const auto& dev) { return isDeviceSuitable(dev); });
        if (deviceIterator == availablePhysicalDevices.end())
        {
            throw std::runtime_error{"failed to find suitable GPU!"};
        }
        physicalDevice = *deviceIterator;
    }

    void createLogicalDevice()
    {
        // QUEUE FAMILY SELECTION //////////////////////////////////////////////////////////////////////////////////////
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        uint32_t queueIndex = ~0;
        for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
        {
            if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
            {
                queueIndex = qfpIndex;
                break;
            }
        }

        if (queueIndex == ~0)
        {
            throw std::runtime_error("Could not find a queue for graphics and present. "
                                     "Terminating...");
        }

        // QUEUE CREATION INFO /////////////////////////////////////////////////////////////////////////////////////////
        float queuePriority{0.5f};

        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
            .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};

        // DEVICE EXTENSIONS ///////////////////////////////////////////////////////////////////////////////////////////
        std::vector<const char*> requiredDeviceExtensions{vk::KHRSwapchainExtensionName};

        // DEVICE FEATURES /////////////////////////////////////////////////////////////////////////////////////////////
        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan11Features,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
            featureChain = {{},
                            {
                                .shaderDrawParameters = true,
                            },
                            {
                                .dynamicRendering = true,
                            },
                            {
                                .extendedDynamicState = true,
                            }};

        // LOGICAL DEVICE CREATION /////////////////////////////////////////////////////////////////////////////////////
        vk::DeviceCreateInfo deviceCreateInfo{.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                              .queueCreateInfoCount = 1,
                                              .pQueueCreateInfos = &deviceQueueCreateInfo,
                                              .enabledExtensionCount =
                                                  static_cast<uint32_t>(requiredDeviceExtensions.size()),
                                              .ppEnabledExtensionNames = requiredDeviceExtensions.data()};

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);

        // Store a handle to the first (0) queue from the queue family `graphicsIndex` on logical device `device`
        graphicsQueue = vk::raii::Queue(device, queueIndex, 0);
    }

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        assert(!availableFormats.empty());

        const auto formatIt = std::ranges::find_if(availableFormats,
                                                   [](const vk::SurfaceFormatKHR& format)
                                                   {
                                                       return format.format == vk::Format::eB8G8R8A8Srgb &&
                                                              format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                                                   });

        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
    }

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        assert(std::ranges::any_of(availablePresentModes,
                                   [](const vk::PresentModeKHR presentMode)
                                   { return presentMode == vk::PresentModeKHR::eFifo; }));
        return std::ranges::any_of(availablePresentModes,
                                   [](const vk::PresentModeKHR value) { return value == vk::PresentModeKHR::eMailbox; })
                   ? vk::PresentModeKHR::eMailbox
                   : vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return {
            .width = std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            .height =
                std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
    }

    uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities)
    {
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        if (0 < surfaceCapabilities.maxImageCount && surfaceCapabilities.maxImageCount < minImageCount)
        {
            minImageCount = surfaceCapabilities.maxImageCount;
        }
        return minImageCount;
    }

    void createSwapChain()
    {
        vk::SurfaceCapabilitiesKHR surfaceCapabilites = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
        swapChainExtent = chooseSwapExtent(surfaceCapabilites);
        uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilites);

        std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
        swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

        std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{
            .surface = *surface,
            .minImageCount = minImageCount,
            .imageFormat = swapChainSurfaceFormat.format,
            .imageColorSpace = swapChainSurfaceFormat.colorSpace,
            .imageExtent = swapChainExtent,
            .imageArrayLayers = 1,                                  // Always 1, unless for stereoscopic 3D.
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment, // Specifies what kind of operations the images in
                                                                    // the swapchain will be used for. In this case,
                                                                    // we'll render directly to them.
            .imageSharingMode = vk::SharingMode::eExclusive, // Specifies how to handle swap chain images that might be
                                                             // used across multiple queue families.
            .preTransform = surfaceCapabilites.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = chooseSwapPresentMode(availablePresentModes),
            .clipped = true};

        swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
        swapChainImages = swapChain.getImages();
    }

    void createImageViews()
    {
        assert(!swapChainImages.empty());

        vk::ImageViewCreateInfo imageViewCreateInfo{
            .viewType = vk::ImageViewType::e2D,
            .format = swapChainSurfaceFormat.format,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                                 .baseMipLevel = 0,
                                 .levelCount = 1,
                                 .baseArrayLayer = 0,
                                 .layerCount = 1},
        };

        for (auto& image : swapChainImages)
        {
            imageViewCreateInfo.image = image;
            swapChainImageViews.emplace_back(device, imageViewCreateInfo);
        }
    }

    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file \"" + filename + "\"");
        }

        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        file.close();

        std::println("File `{}` read {} bytes", filename, buffer.size());

        return buffer;
    }

    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> code) const
    {
        // SPIR-V uses 32-bit encoding, so the code size must be a multiple of sizeof(uint32_t).
        assert(code.size() % sizeof(uint32_t) == 0);

        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t*>(code.data()),
        };

        vk::raii::ShaderModule shaderModule{device, createInfo};

        return shaderModule;
    }

    void createGraphicsPipeline()
    {
        vk::raii::ShaderModule shaderModule = createShaderModule(readFile("shaders/slang.spv"));

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shaderModule,
            .pName = "vertMain",
        };

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shaderModule,
            .pName = "fragMain",
        };

        vk::PipelineShaderStageCreateInfo shaderStages[] = {
            vertShaderStageInfo,
            fragShaderStageInfo,
        };

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{
            .topology = vk::PrimitiveTopology::eTriangleList,
        };

        vk::Viewport viewport{
            .x = 0.f,
            .y = 0.f,
            .width = static_cast<float>(swapChainExtent.width),
            .height = static_cast<float>(swapChainExtent.height),
        };

        vk::Rect2D scissor{vk::Offset2D{0, 0}, swapChainExtent};

        std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data(),
        };

        vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor,
        };

        vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = vk::False,
            .lineWidth = 1.0f,
        };

        vk::PipelineMultisampleStateCreateInfo multisamplingCreateInfo{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = vk::False,
        };

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{
            .blendEnable = vk::True,
            .colorWriteMask = vk::ColorComponentFlagBits::eR |
                              vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB |
                              vk::ColorComponentFlagBits::eA,
        };

        vk::PipelineColorBlendStateCreateInfo blendStateCreateInfo{
            .logicOpEnable = vk::False,
            .logicOp = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
        };

        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            .setLayoutCount = 0,
            .pushConstantRangeCount = 0,
        };

        pipelineLayout = vk::raii::PipelineLayout{device, pipelineLayoutCreateInfo};

        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainSurfaceFormat.format,
        };

        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
            {
                .stageCount = 2,
                .pStages = shaderStages,
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &inputAssemblyCreateInfo,
                .pViewportState = &viewportStateCreateInfo,
                .pRasterizationState = &rasterizationStateCreateInfo,
                .pMultisampleState = &multisamplingCreateInfo,
                .pColorBlendState = &blendStateCreateInfo,
                .pDynamicState = &dynamicStateCreateInfo,
                .layout = pipelineLayout,
                .renderPass = nullptr,
            },
            {
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &swapChainSurfaceFormat.format,
            },
        };

        graphicsPipeline =
            vk::raii::Pipeline{device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()};
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
    vk::raii::SurfaceKHR surface{nullptr};
    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device device{nullptr};
    vk::raii::Queue graphicsQueue{nullptr};
    vk::raii::SwapchainKHR swapChain{nullptr};
    vk::Extent2D swapChainExtent{};
    vk::SurfaceFormatKHR swapChainSurfaceFormat{};
    std::vector<vk::Image> swapChainImages{};
    std::vector<vk::raii::ImageView> swapChainImageViews{};
    vk::raii::PipelineLayout pipelineLayout{nullptr};
    vk::raii::Pipeline graphicsPipeline{nullptr};
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
