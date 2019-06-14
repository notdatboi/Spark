#include"Texture.hpp"

namespace spk
{

    Texture::ImageInfo::ImageInfo()
    {
        type = vk::ImageType::e2D;
        mipLevels = 1;
        arrayLayers = 1;
        samples = vk::SampleCountFlagBits::e1;
        tiling = vk::ImageTiling::eOptimal;
        usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
        uint32_t graphicsFamilyIndex = system::Executives::getInstance()->getGraphicsQueueFamilyIndex();
        sharingMode = vk::SharingMode::eExclusive;
        queueFamilyIndexCount = 1;
        queueFamilyIndices.push_back(graphicsFamilyIndex);
        layout = vk::ImageLayout::eUndefined;
    }

    Texture::ImageInfo::operator vk::ImageCreateInfo()
    {
        vk::ImageCreateInfo info;
        info.setArrayLayers(arrayLayers);
        info.setExtent(extent);
        info.setFlags(flags);
        info.setFormat(format);
        info.setImageType(type);
        info.setInitialLayout(layout);
        info.setMipLevels(mipLevels);
        info.setPQueueFamilyIndices(queueFamilyIndices.data());
        info.setQueueFamilyIndexCount(queueFamilyIndexCount);
        info.setSamples(samples);
        info.setSharingMode(sharingMode);
        info.setTiling(tiling);
        info.setUsage(usage);
        return info;
    }

    Texture& Texture::operator=(const Texture& rTexture)
    {
        destroy();
        create(rTexture.imageInfo.extent.width, rTexture.imageInfo.extent.height, rTexture.imageFormat, rTexture.setIndex, rTexture.binding);
        return *this;
    }

    Texture& Texture::operator=(Texture& rTexture)
    {
        destroy();
        create(rTexture.imageInfo.extent.width, rTexture.imageInfo.extent.height, rTexture.imageFormat, rTexture.setIndex, rTexture.binding);
        return *this;
    }

    void Texture::resetSetIndex(const uint32_t newIndex)
    {
        setIndex = newIndex;
    }

    void Texture::resetBinding(const uint32_t newBinding)
    {
        binding = newBinding;
    }

    const uint32_t Texture::getSet() const
    {
        return setIndex;
    }

    const uint32_t Texture::getBinding() const
    {
        return binding;
    }

    const vk::ImageLayout Texture::getLayout() const
    {
        return image.getLayout();
    }

    Texture::Texture(){}

    Texture::Texture(const Texture& txt)
    {
        create(txt.imageInfo.extent.width, txt.imageInfo.extent.height, txt.imageFormat, txt.setIndex, txt.binding);
    }
    
    Texture::Texture(const uint32_t cWidth, const uint32_t cHeight, ImageFormat cFormat, uint32_t cSetIndex, uint32_t cBinding)
    {
        create(cWidth, cHeight, cFormat, cSetIndex, cBinding);
    }

    void Texture::create(const uint32_t cWidth, const uint32_t cHeight, ImageFormat cFormat, uint32_t cSetIndex, uint32_t cBinding)
    {
        imageFormat = cFormat;
        imageInfo.channelCount = 4;
        switch (cFormat)
        {
        case ImageFormat::RGBA8 :
            imageInfo.format = vk::Format::eR8G8B8A8Unorm;
            break;
        case ImageFormat::BGRA8 :
            imageInfo.format = vk::Format::eB8G8R8A8Unorm;
            break;
        case ImageFormat::RGBA16 :
            imageInfo.format = vk::Format::eR16G16B16A16Unorm;
            break;
        default:
            break;
        }
        imageInfo.extent = vk::Extent3D(cWidth, cHeight, 1);
        setIndex = cSetIndex;
        binding = cBinding;
        image.create({cWidth, cHeight, 1}, imageInfo.format, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::ImageAspectFlagBits::eColor);
        init();
    }

    void Texture::init()
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::CommandPool& commandPool = system::Executives::getInstance()->getPool();

        vk::CommandBufferAllocateInfo commandInfo;
        commandInfo.setCommandBufferCount(1);
        commandInfo.setCommandPool(commandPool);
        commandInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        if(logicalDevice.allocateCommandBuffers(&commandInfo, &layoutChangeCB) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate command buffer!\n");
        if(logicalDevice.allocateCommandBuffers(&commandInfo, &imageUpdateCB) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate command buffer!\n");

        vk::FenceCreateInfo fenceInfo;
        if(logicalDevice.createFence(&fenceInfo, nullptr, &safeToCopyFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");
        if(logicalDevice.createFence(&fenceInfo, nullptr, &contentProcessedFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");
        if(logicalDevice.createFence(&fenceInfo, nullptr, &safeToSampleFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");

        vk::SemaphoreCreateInfo semaphoreInfo;
        if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &safeToCopySemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");
        if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &contentProcessedSemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");
        if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &safeToSampleSemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");
    }

    const vk::ImageView& Texture::getImageView() const 
    {
        return imageView.getView();
    }

    void Texture::bindMemory()
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        image.bindMemory();
        image.changeLayout(layoutChangeCB, vk::ImageLayout::eShaderReadOnlyOptimal, vk::Semaphore(), contentProcessedSemaphore, vk::Fence(), contentProcessedFence);
        if(logicalDevice.waitForFences(1, &contentProcessedFence, true, ~0U) != vk::Result::eSuccess) throw std::runtime_error("Failed to wait for fences!\n");
        if(layoutChangeCB.reset(vk::CommandBufferResetFlags()) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset command buffer!\n");

        imageView.create(image.getImage(), image.getFormat(), image.getSubresource());
    }

    void Texture::update(const void* rawData)
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();

        utils::Buffer rawDataBuffer;
        rawDataBuffer.create(imageInfo.extent.width * imageInfo.extent.height * imageInfo.channelCount, vk::BufferUsageFlagBits::eTransferSrc, false, true);
        rawDataBuffer.bindMemory();
        rawDataBuffer.updateCPUAccessible(rawData);

        image.changeLayout(layoutChangeCB, vk::ImageLayout::eTransferDstOptimal, contentProcessedSemaphore, safeToCopySemaphore, contentProcessedFence, safeToCopyFence);
        image.update(imageUpdateCB, rawDataBuffer.getBuffer(), safeToCopySemaphore, safeToSampleSemaphore, safeToCopyFence, safeToSampleFence, vk::PipelineStageFlagBits::eFragmentShader, true);
        if(layoutChangeCB.reset(vk::CommandBufferResetFlags()) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset command buffer!\n");
        image.changeLayout(layoutChangeCB, vk::ImageLayout::eShaderReadOnlyOptimal, safeToSampleSemaphore, contentProcessedSemaphore, safeToSampleFence, contentProcessedFence);
        if(imageUpdateCB.reset(vk::CommandBufferResetFlagBits::eReleaseResources) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset command buffer!\n");
        if(logicalDevice.waitForFences(1, &contentProcessedFence, true, ~0U) != vk::Result::eSuccess) throw std::runtime_error("Failed to wait for fences!\n");
        if(layoutChangeCB.reset(vk::CommandBufferResetFlags()) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset command buffer!\n");
    }

    void Texture::destroy()
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::CommandPool& pool = system::Executives::getInstance()->getPool();
        if(safeToSampleFence)
        {
            logicalDevice.freeCommandBuffers(pool, 1, &layoutChangeCB);
            layoutChangeCB = vk::CommandBuffer();
            logicalDevice.freeCommandBuffers(pool, 1, &imageUpdateCB);
            imageUpdateCB = vk::CommandBuffer();
            logicalDevice.destroyFence(safeToCopyFence, nullptr);
            safeToCopyFence = vk::Fence();
            logicalDevice.destroyFence(contentProcessedFence, nullptr);
            contentProcessedFence = vk::Fence();
            logicalDevice.destroyFence(safeToSampleFence, nullptr);
            safeToSampleFence = vk::Fence();
            logicalDevice.destroySemaphore(safeToCopySemaphore, nullptr);
            safeToCopySemaphore = vk::Semaphore();
            logicalDevice.destroySemaphore(contentProcessedSemaphore, nullptr);
            contentProcessedSemaphore = vk::Semaphore();
            logicalDevice.destroySemaphore(safeToSampleSemaphore, nullptr);
            safeToSampleSemaphore = vk::Semaphore();
            image.destroy();
            imageView.destroy();
        }
    }

    Texture::~Texture()
    {
        destroy();
    }

}