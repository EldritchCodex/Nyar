/// @file NyarLayer.cppm
/// @brief Vulkan Hello Triangle layer used as Nyar's integration baseline.
/// @details This module follows the Khronos tutorial closely. Nodens owns
///          window and Vulkan platform state while `src/nyar.cppm` evolves
///          toward a renderer API.
/// @ingroup Examples

module;
#include <cassert>
#include <cstdint>

export module NyarLayer;
import Nodens.VulkanContext;
import nodens;
import std;

/// @brief Nodens layer that owns tutorial Vulkan rendering resources.
/// @details Nodens owns the GLFW window and Vulkan platform resources. This layer
///          borrows those resources and owns the pipeline, command, and frame resources.
/// @ingroup Examples
export class NyarLayer : public Nodens::ILayer
{
public:
    /// @brief Constructs an uninitialized layer.
    /// @details `OnAttach()` borrows Nodens Vulkan state before
    ///          creating Nyar rendering resources.
    NyarLayer() {};
    ~NyarLayer() override = default;

    /// @brief Attaches to Nodens Vulkan state and creates rendering resources.
    void OnAttach() override;

    /// @brief Waits for GPU work and releases owned rendering resources.
    void OnDetach() override;

    /// @brief Records and submits one rendered frame.
    /// @param ts Frame delta time supplied by Nodens.
    void OnUpdate(Nodens::TimeStep ts) override;
    // void OnImGuiRender(Nodens::TimeStep ts) override;

private:
    /// @brief Borrows Nodens Vulkan context and swapchain resources.
    void attachToNodensWindow()
    {
        auto& nodensWindow = Nodens::Application::Get().GetWindow();
        auto* graphicsContext = nodensWindow.GetGraphicsContext();
        nodensVulkanContext = dynamic_cast<Nodens::VulkanContext*>(graphicsContext);
        if (!nodensVulkanContext)
            throw std::runtime_error{"NyarLayer requires a Nodens Vulkan window"};

        device = &nodensVulkanContext->GetDeviceRAII();

        swapChainImages = &nodensVulkanContext->GetSwapchainImages();
        swapChainImageViews = &nodensVulkanContext->GetSwapchainImageViews();
        swapChainExtent = nodensVulkanContext->GetSwapchainExtent();
        swapChainSurfaceFormat = nodensVulkanContext->GetSwapchainSurfaceFormat();
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

        vk::raii::ShaderModule shaderModule{*device, createInfo};

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

        pipelineLayout = vk::raii::PipelineLayout{*device, pipelineLayoutCreateInfo};

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
            vk::raii::Pipeline{*device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()};
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
                                           .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                           .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                           .image = swapChainImages->at(imageIndex),
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
        nodensVulkanContext->GetActiveCommandBuffer().pipelineBarrier2(dependency_info);
    }

    /// @brief Records clear, triangle draw, and present transition for one image.
    /// @param imageIndex Swapchain image rendered by the command buffer.
    void recordCommandBuffer(uint32_t imageIndex)
    {
        const auto& commandBuffer = nodensVulkanContext->GetActiveCommandBuffer();

        transition_image_layout(imageIndex,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eColorAttachmentOptimal,
                                {},
                                vk::AccessFlagBits2::eColorAttachmentWrite,
                                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        vk::ClearValue clearColor = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
        vk::RenderingAttachmentInfo renderingAttachmentInfo = {
            .imageView = (*swapChainImageViews)[imageIndex],
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
    }

    /// @brief Records one frame through Nodens.
    void drawFrame()
    {
        const auto imageIndex = nodensVulkanContext->BeginFrame();
        if (!imageIndex)
            return;

        swapChainExtent = nodensVulkanContext->GetSwapchainExtent();
        swapChainSurfaceFormat = nodensVulkanContext->GetSwapchainSurfaceFormat();
        recordCommandBuffer(*imageIndex);
    }

    /// @brief Stops GPU work and releases Vulkan rendering resources.
    void cleanup()
    {
        nodensVulkanContext->WaitIdle();
    }

private:
    Nodens::VulkanContext* nodensVulkanContext{nullptr}; ///< Borrowed Nodens Vulkan context.

    const vk::raii::Device* device{nullptr}; ///< Borrowed logical device owned by Nodens.

    vk::Extent2D swapChainExtent{};                                       ///< Current swapchain dimensions.
    vk::SurfaceFormatKHR swapChainSurfaceFormat{};                        ///< Current swapchain format.
    const std::vector<vk::Image>* swapChainImages{nullptr};               ///< Borrowed swapchain image handles.
    const std::vector<vk::raii::ImageView>* swapChainImageViews{nullptr}; ///< Borrowed image views.
    vk::raii::PipelineLayout pipelineLayout{nullptr};                     ///< Empty tutorial pipeline layout.
    vk::raii::Pipeline graphicsPipeline{nullptr};                         ///< Triangle graphics pipeline.
};
