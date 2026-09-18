module;
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cassert> // Needed for `assert`
#include <cstdio>  // Needed for `stderr`
#include <cstdlib> // Needed for `EXIT_FAILURE` and `EXIT_SUCCESS`
#include <vulkan/vk_platform.h>

export module NyarLayer;
import vulkan;
import nodens;
import std;

constexpr uint32_t WINDOW_WIDTH{100};
constexpr uint32_t WINDOW_HEIGHT{50};
constexpr int MAX_FRAMES_IN_FLIGHT{2};

const std::vector<char const*> validationLayers{"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers{false};
#else
constexpr bool enableValidationLayers{true};
#endif

export class NyarLayer : public Nodens::Layer
{
public:
    NyarLayer(){};
    ~NyarLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Nodens::TimeStep ts) override;
    // void OnImGuiRender(Nodens::TimeStep ts) override;

private:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                          void* pUserData)
    {
        std::println(stderr, "({} / {}) message -> {}", to_string(type), to_string(severity), pCallbackData->pMessage);

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

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto app = reinterpret_cast<NyarLayer*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

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
            features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
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
                                .synchronization2 = true,
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

    void createCommandPool()
    {
        vk::CommandPoolCreateInfo commandPoolCreateInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueIndex,
        };

        commandPool = vk::raii::CommandPool(device, commandPoolCreateInfo);
    }

    void createCommandBuffers()
    {
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
        };

        commandBuffers = std::move(vk::raii::CommandBuffers{device, commandBufferAllocateInfo});
    }

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
        // Because we are using dynamic states for scissor and viewport,
        // we need to set them here.
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

        // LETS DRAW THE TRIANGLE!!!
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

    void cleanupSwapChain()
    {
        swapChainImageViews.clear();
        swapChain = nullptr;
    }

    void recreateSwapChain()
    {
        // Handle window minimization
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

    void cleanup()
    {
        device.waitIdle();
        cleanupSwapChain();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

private:
    // Basic Setup
    GLFWwindow* window{nullptr};
    vk::raii::Context context{};
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
    vk::raii::SurfaceKHR surface{nullptr};

    // Device Setup
    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device device{nullptr};
    vk::raii::Queue graphicsQueue{nullptr};

    // Pipeline Setup
    vk::raii::SwapchainKHR swapChain{nullptr};
    vk::Extent2D swapChainExtent{};
    vk::SurfaceFormatKHR swapChainSurfaceFormat{};
    std::vector<vk::Image> swapChainImages{};
    std::vector<vk::raii::ImageView> swapChainImageViews{};
    vk::raii::PipelineLayout pipelineLayout{nullptr};
    vk::raii::Pipeline graphicsPipeline{nullptr};
    vk::raii::CommandPool commandPool{nullptr};
    std::vector<vk::raii::CommandBuffer> commandBuffers{};
    uint32_t queueIndex{};

    // Synchronization
    std::vector<vk::raii::Semaphore> presentCompleteSemaphores{};
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores{};
    std::vector<vk::raii::Fence> inflightFences{};
    uint32_t frameIndex{0};

    // Resize
    bool framebufferResized{false};
};
