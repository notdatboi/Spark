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

    void Window::destroy()
    {
        const auto& instance = System::getInstance()->getvkInstance();
        instance.destroySurfaceKHR(surface, nullptr);
        glfwDestroyWindow(window);
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

    Window::~Window()
    {
        destroy();
    }

}