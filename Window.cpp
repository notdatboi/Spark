#include"Window.hpp"
#include"System.hpp"

namespace spk
{

    Window::Window(){}

    void Window::create(const uint32_t cWidth, const uint32_t cHeight, const std::string title)
    {
        width = cWidth;
        height = cHeight;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);

        vk::Instance& instance = System::getInstance()->getvkInstance();
        VkSurfaceKHR tmpSurface;
        if(glfwCreateWindowSurface(instance, window, nullptr, &tmpSurface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!\n");
        }
        surface = tmpSurface;

        presentQueue = Executives::getInstance()->getPresentQueue(surface);
        createPresentCommandPool();
        createSwapchain();
        createRenderPass();
        createFramebuffers();
    }

    Window::Window(const uint32_t cWidth, const uint32_t cHeight, const std::string title)
    {
        create(width, height, title);
    }
    
    const vk::SurfaceKHR& Window::getSurface() const
    {
        return surface;
    }

    GLFWwindow* Window::getWindow()
    {
        return window;
    }

    std::pair<uint32_t, const vk::Queue*> Window::getPresentQueue()
    {
        return presentQueue;
    }

    void Window::draw(const ResourceSet* resources, const VertexBuffer* vertexBuffer, const ShaderSet* shaders)
    {
        std::tuple<uint32_t, uint32_t, uint32_t> key = {resources->getIdentifier(), vertexBuffer->getIdentifier(), shaders->getIdentifier()};
        if(drawComponents.count(key) == 0)
        {
            drawComponents[key] = {vk::Pipeline(), resources, vertexBuffer, shaders};
            createPipeline(drawComponents[key].pipeline, shaders->getShaderStages(), vertexBuffer->getAlignmentInfo(), resources->getPipelineLayout());
        }
    }

    void Window::createPipeline(vk::Pipeline& pipeline, const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStageInfos, const VertexAlignmentInfo& vertexAlignmentInfo, const vk::PipelineLayout& layout)
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();

        vk::GraphicsPipelineCreateInfo pipelineInfo;

        pipelineInfo.setStageCount(shaderStageInfos.size());
        pipelineInfo.setPStages(shaderStageInfos.data());
        
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vertexInputInfo.setVertexBindingDescriptionCount(1);
        vk::VertexInputBindingDescription bindingDesc;
        bindingDesc.setBinding(vertexAlignmentInfo.binding);
        bindingDesc.setInputRate(vk::VertexInputRate::eVertex);
        bindingDesc.setStride(vertexAlignmentInfo.structSize);
        vertexInputInfo.setPVertexBindingDescriptions(&bindingDesc);
        
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions(vertexAlignmentInfo.fields.size());
        for(int i = 0; i < attributeDescriptions.size(); ++i)
        {
            attributeDescriptions[i].setLocation(vertexAlignmentInfo.fields[i].location);
            attributeDescriptions[i].setBinding(vertexAlignmentInfo.binding);
            attributeDescriptions[i].setOffset(vertexAlignmentInfo.fields[i].offset);
            switch (vertexAlignmentInfo.fields[i].format)
            {
            case FieldFormat::double64:
                attributeDescriptions[i].setFormat(vk::Format::eR64Sfloat);
                break;
            case FieldFormat::float32 :
                attributeDescriptions[i].setFormat(vk::Format::eR32Sfloat);
                break;
            case FieldFormat::int32 :
                attributeDescriptions[i].setFormat(vk::Format::eR32Sint);
                break;
            case FieldFormat::uint32 :
                attributeDescriptions[i].setFormat(vk::Format::eR32Uint);
                break;
            case FieldFormat::vec2d :
                attributeDescriptions[i].setFormat(vk::Format::eR64G64Sfloat);
                break;
            case FieldFormat::vec2f :
                attributeDescriptions[i].setFormat(vk::Format::eR32G32Sfloat);
                break;
            case FieldFormat::vec2i :
                attributeDescriptions[i].setFormat(vk::Format::eR32G32Sint);
                break;
            case FieldFormat::vec2u :
                attributeDescriptions[i].setFormat(vk::Format::eR32G32Uint);
                break;
            case FieldFormat::vec3d :
                attributeDescriptions[i].setFormat(vk::Format::eR64G64B64Sfloat);
                break;
            case FieldFormat::vec3f :
                attributeDescriptions[i].setFormat(vk::Format::eR32G32B32Sfloat);
                break;
            case FieldFormat::vec3i :
                attributeDescriptions[i].setFormat(vk::Format::eR32G32B32Sint);
                break;
            case FieldFormat::vec3u :
                attributeDescriptions[i].setFormat(vk::Format::eR32G32B32Uint);
                break;
            case FieldFormat::vec4d :
                attributeDescriptions[i].setFormat(vk::Format::eR64G64B64A64Sfloat);
                break;
            case FieldFormat::vec4f :
                attributeDescriptions[i].setFormat(vk::Format::eR32G32B32A32Sfloat);
                break;
            case FieldFormat::vec4i :
                attributeDescriptions[i].setFormat(vk::Format::eR32G32B32A32Sint);
                break;
            case FieldFormat::vec4u :
                attributeDescriptions[i].setFormat(vk::Format::eR32G32B32A32Uint);
                break;
            }
        }
        vertexInputInfo.setVertexAttributeDescriptionCount(attributeDescriptions.size());
        vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

        pipelineInfo.setPVertexInputState(&vertexInputInfo);

        vk::PipelineInputAssemblyStateCreateInfo assemblyInfo;
        assemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList);
        assemblyInfo.setPrimitiveRestartEnable(false);

        pipelineInfo.setPInputAssemblyState(&assemblyInfo);

//        vk::PipelineTessellationStateCreateInfo tesselationInfo;
        pipelineInfo.setPTessellationState(nullptr);

        vk::PipelineViewportStateCreateInfo viewportInfo;
        viewportInfo.setViewportCount(1);
        vk::Viewport vp;
        vp.setX(0);
        vp.setY(0);
        vp.setWidth(width);
        vp.setHeight(height);
        vp.setMinDepth(0.0);
        vp.setMaxDepth(1.0);
        viewportInfo.setPViewports(&vp);
        viewportInfo.setScissorCount(1);
        vk::Rect2D scissor;
        scissor.setOffset({0, 0});
        scissor.setExtent({width, height});
        viewportInfo.setPScissors(&scissor);

        pipelineInfo.setPViewportState(&viewportInfo);

        vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
        rasterizationInfo.setDepthClampEnable(false);
        rasterizationInfo.setRasterizerDiscardEnable(false);
        rasterizationInfo.setPolygonMode(vk::PolygonMode::eFill);
        rasterizationInfo.setCullMode(vk::CullModeFlagBits::eBack);
        rasterizationInfo.setFrontFace(vk::FrontFace::eClockwise);
        rasterizationInfo.setDepthBiasEnable(false);
        rasterizationInfo.setLineWidth(1.0);

        pipelineInfo.setPRasterizationState(&rasterizationInfo);

        vk::PipelineMultisampleStateCreateInfo multisampleInfo;
        multisampleInfo.setSampleShadingEnable(false);
        multisampleInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

        pipelineInfo.setPMultisampleState(&multisampleInfo);

        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
        depthStencilInfo.setDepthTestEnable(false);

        pipelineInfo.setPDepthStencilState(&depthStencilInfo);

        vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        colorBlendInfo.setLogicOpEnable(false);

        vk::PipelineColorBlendAttachmentState attachmentState;
        attachmentState.setBlendEnable(false);
        attachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        colorBlendInfo.setLogicOpEnable(false);
        colorBlendInfo.setAttachmentCount(1);
        colorBlendInfo.setPAttachments(&attachmentState);

        pipelineInfo.setPColorBlendState(&colorBlendInfo);

        /*vk::PipelineDynamicStateCreateInfo dynamicStateInfo;                  // for future
        dynamicStateInfo.setDynamicStateCount(2);
        vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        dynamicStateInfo.setPDynamicStates(dynamicStates);*/
        pipelineInfo.setPDynamicState(nullptr);

        pipelineInfo.setLayout(layout);
        pipelineInfo.setRenderPass(renderPass);
        pipelineInfo.setSubpass(0);

        if(logicalDevice.createGraphicsPipelines(vk::PipelineCache(), 1, &pipelineInfo, nullptr, &pipeline) != vk::Result::eSuccess) throw std::runtime_error("Failed to create pipeline!\n");
    }

    void Window::createSwapchain()
    {
        const vk::PhysicalDevice& physicalDevice = System::getInstance()->getPhysicalDevice();
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        uint32_t graphicsFamilyIndex = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        vk::SurfaceCapabilitiesKHR capabilities;
        if(physicalDevice.getSurfaceCapabilitiesKHR(surface, &capabilities) != vk::Result::eSuccess) throw std::runtime_error("Failed to get surface capabilities!\n");
        uint32_t surfaceFormatsCount;
        physicalDevice.getSurfaceFormatsKHR(surface, &surfaceFormatsCount, nullptr);
        std::vector<vk::SurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
        physicalDevice.getSurfaceFormatsKHR(surface, &surfaceFormatsCount, surfaceFormats.data());
        surfaceFormat = surfaceFormats[0];

        vk::SwapchainCreateInfoKHR swapchainInfo;
        swapchainInfo.setSurface(surface);
        swapchainInfo.setMinImageCount((capabilities.maxImageCount >= 3) ? 3 : capabilities.maxImageCount);
        swapchainInfo.setImageFormat(surfaceFormat.format);
        swapchainInfo.setImageColorSpace(surfaceFormat.colorSpace);
        swapchainInfo.setImageExtent({width, height});
        swapchainInfo.setImageArrayLayers(1);
        swapchainInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
        std::vector<uint32_t> queueFams = {graphicsFamilyIndex};
        if(graphicsFamilyIndex == presentQueue.first)
        {
            swapchainInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        }
        else
        {
            swapchainInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
            queueFams.push_back(presentQueue.first);
        }
        swapchainInfo.setQueueFamilyIndexCount(queueFams.size());
        swapchainInfo.setPQueueFamilyIndices(queueFams.data());
        swapchainInfo.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        swapchainInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        swapchainInfo.setPresentMode(vk::PresentModeKHR::eFifo);
        swapchainInfo.setClipped(true);

        if(logicalDevice.createSwapchainKHR(&swapchainInfo, nullptr, &swapchain) != vk::Result::eSuccess) throw std::runtime_error("Failed to create swapchain!\n");

        uint32_t swapchainImgCount;
        if(logicalDevice.getSwapchainImagesKHR(swapchain, &swapchainImgCount, nullptr) != vk::Result::eSuccess) throw std::runtime_error("Failed to get swapchain images!\n");
        swapchainImages.resize(swapchainImgCount);
        if(logicalDevice.getSwapchainImagesKHR(swapchain, &swapchainImgCount, swapchainImages.data()) != vk::Result::eSuccess) throw std::runtime_error("Failed to get swapchain images!\n");

        for(auto& img : swapchainImages)
        {
            vk::ImageSubresourceRange range;
            range.setAspectMask(vk::ImageAspectFlagBits::eColor);
            range.setBaseMipLevel(0);
            range.setLevelCount(1);
            range.setBaseArrayLayer(0);
            range.setLayerCount(1);

            vk::ImageViewCreateInfo viewInfo;
            viewInfo.setImage(img);
            viewInfo.setViewType(vk::ImageViewType::e2D);
            viewInfo.setFormat(surfaceFormat.format);
            viewInfo.setComponents(vk::ComponentMapping());
            viewInfo.setSubresourceRange(range);

            swapchainImageViews.push_back(vk::ImageView());
            if(logicalDevice.createImageView(&viewInfo, nullptr, &(*(swapchainImageViews.end() - 1))) != vk::Result::eSuccess) throw std::runtime_error("Failed to create image view!\n");
        }
    }

    void Window::createRenderPass()
    {
        const vk::Device& logicalDevice = spk::System::getInstance()->getLogicalDevice();

        vk::AttachmentDescription attachment;
        attachment.setFormat(surfaceFormat.format);
        attachment.setSamples(vk::SampleCountFlagBits::e1);
        attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        attachment.setInitialLayout(vk::ImageLayout::eUndefined);          
        attachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        vk::AttachmentReference attachmentReference;
        attachmentReference.setAttachment(0);
        attachmentReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription subpassDesc;
        subpassDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
        subpassDesc.setInputAttachmentCount(0);
        subpassDesc.setColorAttachmentCount(1);
        subpassDesc.setPColorAttachments(&attachmentReference);
        subpassDesc.setPResolveAttachments(nullptr);
        subpassDesc.setPDepthStencilAttachment(nullptr);
        subpassDesc.setPreserveAttachmentCount(0);

        vk::RenderPassCreateInfo info;
        info.setAttachmentCount(1);
        info.setPAttachments(&attachment);
        info.setSubpassCount(1);
        info.setPSubpasses(&subpassDesc);
        info.setDependencyCount(0);

        if(logicalDevice.createRenderPass(&info, nullptr, &renderPass) != vk::Result::eSuccess) throw std::runtime_error("Failed to create render pass!\n");
    }

    void Window::createFramebuffers()
    {
        const vk::Device& logicalDevice = spk::System::getInstance()->getLogicalDevice();
        framebuffers.resize(swapchainImageViews.size());
        int i = 0;
        for(auto& fb : framebuffers)
        {
            vk::FramebufferCreateInfo info;
            info.setRenderPass(renderPass);
            info.setAttachmentCount(1);
            info.setPAttachments(&swapchainImageViews[i]);
            info.setWidth(width);
            info.setHeight(height);
            info.setLayers(1);
            if(logicalDevice.createFramebuffer(&info, nullptr, &fb) != vk::Result::eSuccess) throw std::runtime_error("Failed to create framebuffer!\n");
            ++i;
        }
    }

    void Window::createPresentCommandPool()
    {
        const vk::Device& logicalDevice = spk::System::getInstance()->getLogicalDevice();
        vk::CommandPoolCreateInfo info;
        info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        info.setQueueFamilyIndex(presentQueue.first);
        if(logicalDevice.createCommandPool(&info, nullptr, &presentCommandPool) != vk::Result::eSuccess) throw std::runtime_error("Failed to create command pool!\n");
    }

    void Window::destroy()
    {
        const auto& instance = System::getInstance()->getvkInstance();
        const vk::Device& logicalDevice = spk::System::getInstance()->getLogicalDevice();
        logicalDevice.destroyCommandPool(presentCommandPool, nullptr);
        instance.destroySurfaceKHR(surface, nullptr);
        glfwDestroyWindow(window);
    }

    Window::~Window()
    {
        destroy();
    }

}