#include"Window.hpp"
#include"System.hpp"

namespace spk
{

    Window::Window(){}

    void Window::create(const uint32_t cWidth, const uint32_t cHeight, const std::string cTitle, const DrawOptions cOptions)
    {
        currentPipeline = {~uint32_t(0), ~uint32_t(0), ~uint32_t(0)};
        width = cWidth;
        height = cHeight;
        options = cOptions;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), cTitle.c_str(), nullptr, nullptr);

        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        vk::SemaphoreCreateInfo semaphoreInfo;
        logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &safeToPresentSemaphore);
        logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &safeToRenderSemaphore);
        vk::FenceCreateInfo fenceInfo;
        logicalDevice.createFence(&fenceInfo, nullptr, &safeToRenderFence);
        logicalDevice.createFence(&fenceInfo, nullptr, &safeToPresentFence);

        vk::Instance& instance = system::System::getInstance()->getvkInstance();
        VkSurfaceKHR tmpSurface;
        if(glfwCreateWindowSurface(instance, window, nullptr, &tmpSurface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!\n");
        }
        surface = tmpSurface;

        presentQueue = system::Executives::getInstance()->getPresentQueue(surface);
        createSwapchain();
        createCommandBuffers();
        createRenderPass();
        createFramebuffers();
    }

    Window::Window(const uint32_t cWidth, const uint32_t cHeight, const std::string cTitle, const DrawOptions cOptions)
    {
        create(width, height, cTitle, cOptions);
    }
    
    GLFWwindow* Window::getGLFWWindow()
    {
        return window;
    }

    void Window::draw(const ResourceSet* resources, const VertexBuffer* vertexBuffer, const ShaderSet* shaders)
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::Queue& graphicsQueue = system::Executives::getInstance()->getGraphicsQueue();
        std::tuple<uint32_t, uint32_t, uint32_t> key = {resources->getIdentifier(), vertexBuffer->getIdentifier(), shaders->getIdentifier()};
        if(drawComponents.count(key) == 0)
        {
            drawComponents[key] = {vk::Pipeline(), resources, vertexBuffer, shaders};
            createPipeline(drawComponents[key].pipeline, shaders->getShaderStages(), vertexBuffer->getAlignmentInfo(), resources->getPipelineLayout());
        }
        if(currentPipeline != key)
        {
            graphicsQueue.waitIdle();
            currentPipeline = key;
            if(frameCommandBuffers[0])
            {
                for(auto& cb : frameCommandBuffers)
                {
                    cb.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
                }
            }
            initCommandBuffers(drawComponents[key]);
        }
        uint32_t imageIndex;
        if(logicalDevice.acquireNextImageKHR(swapchain, ~0U, safeToRenderSemaphore, safeToRenderFence, &imageIndex) != vk::Result::eSuccess) throw std::runtime_error("Failed to acquire image!\n");

        vk::PipelineStageFlags renderStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo renderSubmit;
        renderSubmit.setCommandBufferCount(1);
        renderSubmit.setPCommandBuffers(&frameCommandBuffers[imageIndex]);
        renderSubmit.setSignalSemaphoreCount(1);
        renderSubmit.setPSignalSemaphores(&safeToPresentSemaphore);
        renderSubmit.setWaitSemaphoreCount(1);
        renderSubmit.setPWaitSemaphores(&safeToRenderSemaphore);
        renderSubmit.setPWaitDstStageMask(&renderStageFlags);

        if(logicalDevice.waitForFences(1, &safeToRenderFence, true, ~0U) != vk::Result::eSuccess) throw std::runtime_error("Failed to wait for fences!\n");

        if(graphicsQueue.submit(1, &renderSubmit, safeToPresentFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to submit queue!\n");

        vk::Result presentationResult;
        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphoreCount(1);
        presentInfo.setPWaitSemaphores(&safeToPresentSemaphore);
        presentInfo.setSwapchainCount(1);
        presentInfo.setPSwapchains(&swapchain);
        presentInfo.setPImageIndices(&imageIndex);
        presentInfo.setPResults(&presentationResult);

        if(logicalDevice.waitForFences(1, &safeToPresentFence, true, ~0U) != vk::Result::eSuccess) throw std::runtime_error("Failed to wait for fences!\n");

        presentQueue.second->presentKHR(&presentInfo);
        if(presentationResult != vk::Result::eSuccess) throw std::runtime_error("Failed to perform presentation!\n");

        logicalDevice.resetFences(1, &safeToRenderFence);
        logicalDevice.resetFences(1, &safeToPresentFence);
    }

    void Window::initCommandBuffers(DrawComponents& drawComponents)
    {
        const vk::CommandPool& commandPool = system::Executives::getInstance()->getPool();
        int i = 0;
        for(auto& commandBuffer : frameCommandBuffers)
        {
            vk::CommandBufferBeginInfo beginInfo;
            if(commandBuffer.begin(&beginInfo) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, drawComponents.pipeline);

            vk::ClearValue clear;
            vk::ClearColorValue clearColorValue;
            vk::ClearDepthStencilValue clearDSValue;
            clearColorValue.setUint32({0, 0, 0, 0});
            clear.setColor(clearColorValue);
            clearDSValue.setDepth(0);
            clearDSValue.setStencil(0);
            clear.setDepthStencil(clearDSValue);

            vk::RenderPassBeginInfo renderPassInfo;
            renderPassInfo.setRenderPass(renderPass);
            renderPassInfo.setFramebuffer(framebuffers[i]);
            renderPassInfo.setRenderArea(vk::Rect2D({0, 0}, {width, height}));
            renderPassInfo.setClearValueCount(1);
            renderPassInfo.setPClearValues(&clear);
            commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents());

            vk::DeviceSize offset = 0;
            const vk::Buffer& vb = drawComponents.vertices->getVertexBuffer();
            const uint32_t vbSize = drawComponents.vertices->getVertexBufferSize();
            const uint32_t ibSize = drawComponents.vertices->getIndexBufferSize();
            const VertexAlignmentInfo alignmentInfo = drawComponents.vertices->getAlignmentInfo();
            commandBuffer.bindVertexBuffers(alignmentInfo.binding, 1, &vb, &offset);
            if(ibSize != 0)
            {
                const vk::Buffer& ib = drawComponents.vertices->getIndexBuffer();
                commandBuffer.bindIndexBuffer(ib, offset, vk::IndexType::eUint32);
            }
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, drawComponents.resources->getPipelineLayout(), 0, drawComponents.resources->getDescriptorSets().size(), drawComponents.resources->getDescriptorSets().data(), 0, nullptr);

            if(ibSize != 0)
            {
                commandBuffer.drawIndexed(ibSize / 4, 1, 0, 0, 0);
            }
            else
            {
                commandBuffer.draw(vbSize / alignmentInfo.structSize, 1, 0, 0);
            }

            commandBuffer.endRenderPass();
            commandBuffer.end();
            ++i;
        }
    }

    void Window::createPipeline(vk::Pipeline& pipeline, const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStageInfos, const VertexAlignmentInfo& vertexAlignmentInfo, const vk::PipelineLayout& layout)
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();

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
        switch (options.cullMode)
        {
        case CullMode::None :
            rasterizationInfo.setCullMode(vk::CullModeFlagBits::eNone);
            break;
        case CullMode::CounterClockwise :
            rasterizationInfo.setCullMode(vk::CullModeFlagBits::eBack);
            rasterizationInfo.setFrontFace(vk::FrontFace::eClockwise);
            break;
        case CullMode::Clockwise :
            rasterizationInfo.setCullMode(vk::CullModeFlagBits::eBack);
            rasterizationInfo.setFrontFace(vk::FrontFace::eCounterClockwise);
            break;
        
        default:
            break;
        }
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
        const vk::PhysicalDevice& physicalDevice = system::System::getInstance()->getPhysicalDevice();
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        uint32_t graphicsFamilyIndex = system::Executives::getInstance()->getGraphicsQueueFamilyIndex();
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
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();

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
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
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

    void Window::createCommandBuffers()
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::CommandPool& pool = system::Executives::getInstance()->getPool();
        frameCommandBuffers.resize(swapchainImages.size());
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.setCommandBufferCount(frameCommandBuffers.size());
        allocInfo.setCommandPool(pool);
        allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        logicalDevice.allocateCommandBuffers(&allocInfo, frameCommandBuffers.data());
    }

    void Window::destroy()
    {
        if(safeToPresentSemaphore)
        {
            const auto& instance = system::System::getInstance()->getvkInstance();
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            logicalDevice.waitIdle();
            logicalDevice.destroySemaphore(safeToPresentSemaphore, nullptr);
            safeToPresentSemaphore = vk::Semaphore();
            logicalDevice.destroySemaphore(safeToRenderSemaphore, nullptr);
            logicalDevice.destroyFence(safeToPresentFence, nullptr);
            logicalDevice.destroyFence(safeToRenderFence, nullptr);
            for(auto& fb : framebuffers)
            {
                logicalDevice.destroyFramebuffer(fb, nullptr);
            }
            for(auto& component : drawComponents)
            {
                logicalDevice.destroyPipeline(component.second.pipeline, nullptr);
            }
            logicalDevice.destroyRenderPass(renderPass, nullptr);
            for(vk::ImageView& view : swapchainImageViews)
            {
                logicalDevice.destroyImageView(view, nullptr);
            }
            logicalDevice.destroySwapchainKHR(swapchain, nullptr);
            instance.destroySurfaceKHR(surface, nullptr);
            glfwDestroyWindow(window);
        }
    }

    Window::~Window()
    {
        destroy();
    }

}