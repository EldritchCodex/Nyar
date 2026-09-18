/// @file NyarLayer.cppm
/// @brief Vulkan Hello Triangle layer used as Nyar's integration baseline.
/// @details This module follows the Khronos tutorial closely. It owns the
///          temporary GLFW/Vulkan path while `src/nyar.cppm` evolves toward
///          a renderer API.
/// @ingroup Examples

module;
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vulkan/vk_platform.h>

export module NyarLayer;
import vulkan;
import nodens;
import std;

constexpr uint32_t WINDOW_WIDTH{100};  ///< Initial window width in pixels.
constexpr uint32_t WINDOW_HEIGHT{50};  ///< Initial window height in pixels.
constexpr int MAX_FRAMES_IN_FLIGHT{2}; ///< Number of CPU frames allowed in flight.

const std::vector<char const*> validationLayers{
    "VK_LAYER_KHRONOS_validation"}; ///< Validation layer used by debug builds.

#ifdef NDEBUG
constexpr bool enableValidationLayers{false};
#else
constexpr bool enableValidationLayers{true};
#endif

/// @brief Nodens layer that owns tutorial window and Vulkan resources.
/// @details Resource creation follows Vulkan dependency order. RAII handles
///          release most objects, while `cleanup()` waits for GPU work before
///          destroying GLFW.
/// @ingroup Examples
export class NyarLayer : public Nodens::ILayer
{
public:
    /// @brief Constructs an uninitialized layer.
    /// @details `OnAttach()` performs window and Vulkan setup.
    NyarLayer() {};
    ~NyarLayer() override = default;

    /// @brief Creates window, Vulkan objects, pipeline, command buffers, and sync state.
    void OnAttach() override;

    /// @brief Waits for GPU work and releases owned window/Vulkan resources.
    void OnDetach() override;

    /// @brief Polls window events and renders one frame.
    /// @param ts Frame delta time supplied by Nodens.
    void OnUpdate(Nodens::TimeStep ts) override;
    // void OnImGuiRender(Nodens::TimeStep ts) override;

private:
    /// @brief Prints validation messages without requesting callback termination.
    /// @param severity Message severity reported by Vulkan.
    /// @param type Message categories reported by Vulkan.
    /// @param pCallbackData Message text and metadata.
    /// @param pUserData Reserved user data pointer.
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                          void* pUserData)
    {
        std::println(stderr, "({} / {}) message -> {}", to_string(type), to_string(severity), pCallbackData->pMessage);

        return vk::False;
    }

    /// @brief Collects GLFW surface extensions and optional debug-utils extension.
    /// @return Extension names required to create the instance.
    std::vector<const char*> getRequiredInstanceExtensionsNames()
    {
        // GLFW determines platform-specific extensions required by its surface.
        uint32_t glfwExtensionCount{0};
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    }

    /// @brief Validates requested extensions/layers, then creates Vulkan instance.
    void createInstance()
    {
        // Application metadata and requested Vulkan API version.
        constexpr vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
                                              .applicationVersion = vk::makeApiVersion(0, 0, 0, 0),
                                              .pEngineName = "Nyar",
                                              .engineVersion = vk::makeApiVersion(0, 0, 0, 0),
                                              .apiVersion = vk::ApiVersion14};

        // Enumerate extensions before checking GLFW and debug-utils requirements.
        auto availableExtensionsNames =
            context.enumerateInstanceExtensionProperties() |
            std::views::transform([](const auto& extentionProperty)
                                  { return std::string_view{extentionProperty.extensionName}; });
        std::println("Available extensions:");
        for (auto name : availableExtensionsNames)
        {
            std::println("\t{}", name);
        }

        // Check required extensions.
        auto requiredExtensionsNames = getRequiredInstanceExtensionsNames();
        auto unsupportedExtensionIt =
            std::ranges::find_if(requiredExtensionsNames,
                                 [&availableExtensionsNames](std::string_view requiredName)
                                 { return !std::ranges::contains(availableExtensionsNames, requiredName); });
        if (unsupportedExtensionIt != requiredExtensionsNames.end())
        {
            throw std::runtime_error{"Required GLFW extension not supported: " + std::string{*unsupportedExtensionIt}};
        }

        // Enumerate layers before enabling validation.
        auto layersNames =
            context.enumerateInstanceLayerProperties() |
            std::views::transform([](const auto& layerProperty) { return std::string_view{layerProperty.layerName}; });
        std::println("Available layers:");
        for (auto name : layersNames)
        {
            std::println("\t{}", name);
        }

        // Check required layers.
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

        // Create instance after all requested capabilities pass validation.
        vk::InstanceCreateInfo createInfo{.pApplicationInfo = &appInfo,
                                          .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
                                          .ppEnabledLayerNames = requiredLayers.data(),
                                          .enabledExtensionCount =
                                              static_cast<uint32_t>(requiredExtensionsNames.size()),
                                          .ppEnabledExtensionNames = requiredExtensionsNames.data()};

        instance = vk::raii::Instance{context, createInfo};
    }

    /// @brief Marks swapchain for recreation after framebuffer size changes.
    /// @param window GLFW window whose framebuffer changed.
    /// @param width New framebuffer width in pixels.
    /// @param height New framebuffer height in pixels.
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto app = reinterpret_cast<NyarLayer*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    /// @brief Initializes GLFW and creates a context-free Vulkan window.
    void initWindow()
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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    /// @brief Installs validation callback when validation layers are enabled.
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

    /// @brief Bridges GLFW's native handle into a Vulkan presentation surface.
    void createSurface()
    {
        VkSurfaceKHR rawSurface{VK_NULL_HANDLE};
        if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != 0)
        {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::raii::SurfaceKHR(instance, rawSurface);
    }

    /// @brief Checks API version, queue support, extensions, and features.
    /// @param physicalDevice Candidate device to inspect.
    /// @return True when device satisfies example requirements.
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
            features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
    }

    /// @brief Selects first physical device satisfying `isDeviceSuitable()`.
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

    /// @brief Creates logical device with one graphics-and-present queue.
    void createLogicalDevice()
    {
        // Select one queue family that supports graphics and presentation.
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        queueIndex = ~0;
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

        // Describe queue priority and selected queue family.
        float queuePriority{0.5f};

        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
            .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};

        // Enable swapchain support on logical device.
        std::vector<const char*> requiredDeviceExtensions{vk::KHRSwapchainExtensionName};

        // Enable features used by dynamic rendering and synchronization2.
        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan11Features,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
            featureChain = {{},
                            {
                                .shaderDrawParameters = true,
                            },
                            {
                                .synchronization2 = true,
                                .dynamicRendering = true,
                            },
                            {
                                .extendedDynamicState = true,
                            }};

        // Create device and retrieve its graphics/present queue.
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

    /// @brief Prefers sRGB color; falls back to first surface format.
    /// @param availableFormats Formats reported by the presentation surface.
    /// @return Format used by the swapchain.
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

    /// @brief Prefers mailbox presentation; uses required FIFO fallback.
    /// @param availablePresentModes Modes reported by the presentation surface.
    /// @return Presentation mode used by the swapchain.
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

    /// @brief Chooses surface extent, clamping GLFW framebuffer size when needed.
    /// @param capabilities Surface limits and current extent.
    /// @return Extent used by the swapchain.
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

    /// @brief Chooses at least three swapchain images within surface limits.
    /// @param surfaceCapabilities Surface limits for image count.
    /// @return Image count used by the swapchain.
    uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities)
    {
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        if (0 < surfaceCapabilities.maxImageCount && surfaceCapabilities.maxImageCount < minImageCount)
        {
            minImageCount = surfaceCapabilities.maxImageCount;
        }
        return minImageCount;
    }

    /// @brief Creates swapchain using current capabilities and preferences.
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

    /// @brief Creates one color image view for each swapchain image.
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

    /// @brief Reads binary shader data relative to process working directory.
    /// @param filename Shader file path.
    /// @return File contents as bytes.
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

    /// @brief Creates shader module from validated SPIR-V bytecode.
    /// @param code SPIR-V bytecode.
    /// @return RAII shader-module handle.
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> code) const
    {
        // SPIR-V uses 32-bit words, so bytecode size must be word-aligned.
        assert(code.size() % sizeof(uint32_t) == 0);

        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t*>(code.data()),
        };

        vk::raii::ShaderModule shaderModule{device, createInfo};

        return shaderModule;
    }

    /// @brief Creates graphics pipeline for dynamic rendering and triangle draw.
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
            .blendEnable = vk::False,
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

    /// @brief Creates resettable command pool for selected graphics queue family.
    void createCommandPool()
    {
        vk::CommandPoolCreateInfo commandPoolCreateInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueIndex,
        };

        commandPool = vk::raii::CommandPool(device, commandPoolCreateInfo);
    }

    /// @brief Allocates one primary command buffer per frame in flight.
    void createCommandBuffers()
    {
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
        };

        commandBuffers = std::move(vk::raii::CommandBuffers{device, commandBufferAllocateInfo});
    }

    /// @brief Inserts synchronization2 barrier for one swapchain image transition.
    /// @param imageIndex Swapchain image to transition.
    /// @param old_layout Current image layout.
    /// @param new_layout Required image layout.
    /// @param src_access_mask Access scope before transition.
    /// @param dst_access_mask Access scope after transition.
    /// @param src_stage_mask Pipeline stage before transition.
    /// @param dst_stage_mask Pipeline stage after transition.
    void transition_image_layout(uint32_t imageIndex,
                                 vk::ImageLayout old_layout,
                                 vk::ImageLayout new_layout,
                                 vk::AccessFlags2 src_access_mask,
                                 vk::AccessFlags2 dst_access_mask,
                                 vk::PipelineStageFlags2 src_stage_mask,
                                 vk::PipelineStageFlags2 dst_stage_mask)
    {
        vk::ImageMemoryBarrier2 barrier = {.srcStageMask = src_stage_mask,
                                           .srcAccessMask = src_access_mask,
                                           .dstStageMask = dst_stage_mask,
                                           .dstAccessMask = dst_access_mask,
                                           .oldLayout = old_layout,
                                           .newLayout = new_layout,
                                           .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                           .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                           .image = swapChainImages[imageIndex],
                                           .subresourceRange = {
                                               .aspectMask = vk::ImageAspectFlagBits::eColor,
                                               .baseMipLevel = 0,
                                               .levelCount = 1,
                                               .baseArrayLayer = 0,
                                               .layerCount = 1,
                                           }};
        vk::DependencyInfo dependency_info = {
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
        };
        commandBuffers[frameIndex].pipelineBarrier2(dependency_info);
    }

    /// @brief Records clear, triangle draw, and present transition for one image.
    /// @param imageIndex Swapchain image rendered by the command buffer.
    void recordCommandBuffer(uint32_t imageIndex)
    {
        auto& commandBuffer = commandBuffers[frameIndex];
        commandBuffer.begin({});

        transition_image_layout(imageIndex,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eColorAttachmentOptimal,
                                {},
                                vk::AccessFlagBits2::eColorAttachmentWrite,
                                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        vk::ClearValue clearColor = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
        vk::RenderingAttachmentInfo renderingAttachmentInfo = {
            .imageView = swapChainImageViews[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor,
        };

        vk::RenderingInfo renderingInfo = {
            .renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &renderingAttachmentInfo,
        };

        commandBuffer.beginRendering(renderingInfo);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        // Dynamic viewport and scissor state must be set during recording.
        commandBuffer.setViewport(0,
                                  vk::Viewport{
                                      .x = 0.f,
                                      .y = 0.f,
                                      .width = static_cast<float>(swapChainExtent.width),
                                      .height = static_cast<float>(swapChainExtent.height),
                                      .minDepth = 0.f,
                                      .maxDepth = 1.f,
                                  });
        commandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, swapChainExtent});

        // Vertex shader generates triangle vertices from vertex index.
        commandBuffer.draw(3, 1, 0, 0);

        commandBuffer.endRendering();

        transition_image_layout(imageIndex,
                                vk::ImageLayout::eColorAttachmentOptimal,
                                vk::ImageLayout::ePresentSrcKHR,
                                vk::AccessFlagBits2::eColorAttachmentWrite,
                                {},
                                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                vk::PipelineStageFlagBits2::eBottomOfPipe);

        commandBuffer.end();
    }

    /// @brief Creates acquire/present semaphores and per-frame fences.
    void createSyncObjects()
    {
        assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inflightFences.empty());

        for (size_t i = 0; i < swapChainImages.size(); ++i)
        {
            renderFinishedSemaphores.push_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo{}));
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            presentCompleteSemaphores.push_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo{}));
            inflightFences.push_back(
                vk::raii::Fence(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}));
        }
    }

    /// @brief Acquires, records, submits, and presents one frame.
    void drawFrame()
    {
        auto fenceResult =
            device.waitForFences(*inflightFences[frameIndex], vk::True, std::numeric_limits<uint64_t>::max());
        if (fenceResult != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to wait for fence");
        }

        auto [result, imageIndex] = swapChain.acquireNextImage(
            std::numeric_limits<uint64_t>::max(), *presentCompleteSemaphores[frameIndex], nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapChain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            throw std::runtime_error("failed to acquire swap chain image");
        }

        device.resetFences(*inflightFences[frameIndex]);

        commandBuffers[frameIndex].reset();
        recordCommandBuffer(imageIndex);

        vk::PipelineStageFlags waitDestinationStageMask{vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::SubmitInfo submitInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
            .pWaitDstStageMask = &waitDestinationStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffers[frameIndex],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex],
        };

        graphicsQueue.submit(submitInfo, *inflightFences[frameIndex]);

        const vk::PresentInfoKHR presentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
            .swapchainCount = 1,
            .pSwapchains = &*swapChain,
            .pImageIndices = &imageIndex,
        };

        result = graphicsQueue.presentKHR(presentInfoKHR);

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
            return;
        }
        else
        {
            assert(result == vk::Result::eSuccess);
        }

        frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    /// @brief Releases resources tied to the current swapchain.
    void cleanupSwapChain()
    {
        swapChainImageViews.clear();
        swapChain = nullptr;
    }

    /// @brief Rebuilds swapchain and image views after surface changes.
    void recreateSwapChain()
    {
        // Wait until minimized window has a non-zero framebuffer.
        int width{0}, height{0};
        glfwGetFramebufferSize(window, &width, &height);
        while ((width == 0 || height == 0) && !glfwWindowShouldClose(window))
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        if (glfwWindowShouldClose(window))
        {
            return;
        }

        device.waitIdle();

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
    }

    /// @brief Stops GPU work, releases swapchain resources, and terminates GLFW.
    void cleanup()
    {
        device.waitIdle();
        cleanupSwapChain();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

private:
    GLFWwindow* window{nullptr};                              ///< GLFW window owned by this layer.
    vk::raii::Context context{};                              ///< Vulkan loader context.
    vk::raii::Instance instance{nullptr};                     ///< Vulkan instance.
    vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr}; ///< Validation callback.
    vk::raii::SurfaceKHR surface{nullptr};                    ///< Presentation surface created from GLFW.

    vk::raii::PhysicalDevice physicalDevice{nullptr}; ///< Selected physical device.
    vk::raii::Device device{nullptr};                 ///< Logical device exposing required features.
    vk::raii::Queue graphicsQueue{nullptr};           ///< Queue supporting graphics and presentation.

    vk::raii::SwapchainKHR swapChain{nullptr};              ///< Images presented to the window.
    vk::Extent2D swapChainExtent{};                         ///< Current swapchain dimensions.
    vk::SurfaceFormatKHR swapChainSurfaceFormat{};          ///< Current swapchain format.
    std::vector<vk::Image> swapChainImages{};               ///< Swapchain image handles.
    std::vector<vk::raii::ImageView> swapChainImageViews{}; ///< Swapchain color views.
    vk::raii::PipelineLayout pipelineLayout{nullptr};       ///< Empty tutorial pipeline layout.
    vk::raii::Pipeline graphicsPipeline{nullptr};           ///< Triangle graphics pipeline.
    vk::raii::CommandPool commandPool{nullptr};             ///< Pool for graphics command buffers.
    std::vector<vk::raii::CommandBuffer> commandBuffers{};  ///< Per-frame command buffers.
    uint32_t queueIndex{};                                  ///< Selected graphics/presentation queue family.

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores{}; ///< Image-acquire signals.
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores{};  ///< Render-complete signals.
    std::vector<vk::raii::Fence> inflightFences{};                ///< CPU/GPU frame fences.
    uint32_t frameIndex{0};                                       ///< Current frame-in-flight index.

    bool framebufferResized{false}; ///< Set by GLFW callback and consumed by drawFrame().
};
