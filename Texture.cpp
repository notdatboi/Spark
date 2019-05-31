#include"Texture.hpp"

namespace spk
{

    Texture::ImageInfo::ImageInfo()
    {
        type = vk::ImageType::e2D;
        //format = vk::Format::eR8G8B8A8Unorm;
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
        //channelCount = 4;
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

    Texture& Texture::operator=(Texture&& rTexture)
    {
        destroy();
        rTexture.transferred = true;
        imageInfo = std::move(rTexture.imageInfo);
        imageFormat = std::move(rTexture.imageFormat);
        memoryData = std::move(rTexture.memoryData);
        image = std::move(rTexture.image);
        view = std::move(rTexture.view);
        textureReadyFence = std::move(rTexture.textureReadyFence);
        textureReadySemaphore = std::move(rTexture.textureReadySemaphore);
        rawImageData = std::move(rTexture.rawImageData);
        binding = std::move(rTexture.binding);
        return *this;
    }

    const vk::Semaphore* Texture::getSemaphore() const
    {
        return &textureReadySemaphore;
    }

    const vk::Fence* Texture::getFence() const
    {
        return &textureReadyFence;
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

    const vk::ImageLayout& Texture::getLayout() const
    {
        return imageInfo.layout;
    }

    Texture::Texture(){}

    Texture::Texture(const Texture& txt)
    {
        create(txt.imageInfo.extent.width, txt.imageInfo.extent.height, txt.imageFormat, txt.setIndex, txt.binding);
    }
    
    Texture::Texture(Texture&& txt)
    {
        txt.transferred = true;
        imageInfo = std::move(txt.imageInfo);
        imageFormat = std::move(txt.imageFormat);
        memoryData = std::move(txt.memoryData);
        image = std::move(txt.image);
        view = std::move(txt.view);
        textureReadyFence = std::move(txt.textureReadyFence);
        textureReadySemaphore = std::move(txt.textureReadySemaphore);
        rawImageData = std::move(txt.rawImageData);
        binding = std::move(txt.binding);
    }

    Texture::Texture(const uint32_t cWidth, const uint32_t cHeight, ImageFormat cFormat, uint32_t cSetIndex, uint32_t cBinding)
    {
        create(cWidth, cHeight, cFormat, cSetIndex, cBinding);
    }

    void Texture::create(const uint32_t cWidth, const uint32_t cHeight, ImageFormat cFormat, uint32_t cSetIndex, uint32_t cBinding)
    {
        imageFormat = cFormat;
        switch (cFormat)
        {
        case ImageFormat::RGBA8 :
            imageInfo.format = vk::Format::eR8G8B8A8Unorm;
            imageInfo.channelCount = 4;
            break;
        case ImageFormat::RGB8 :
            imageInfo.format = vk::Format::eR8G8B8Unorm;
            imageInfo.channelCount = 3;
            break;
        case ImageFormat::BGR8 :
            imageInfo.format = vk::Format::eB8G8R8Unorm;
            imageInfo.channelCount = 3;
            break;
        case ImageFormat::BGRA8 :
            imageInfo.format = vk::Format::eB8G8R8A8Unorm;
            imageInfo.channelCount = 4;
            break;
        case ImageFormat::RGB16 :
            imageInfo.format = vk::Format::eR16G16B16Unorm;
            imageInfo.channelCount = 3;
            break;
        case ImageFormat::RGBA16 :
            imageInfo.format = vk::Format::eR16G16B16A16Unorm;
            imageInfo.channelCount = 4;
            break;
        default:
            break;
        }
        imageInfo.extent = vk::Extent3D(cWidth, cHeight, 1);
        rawImageData.resize(cWidth * cHeight * imageInfo.channelCount);
        setIndex = cSetIndex;
        binding = cBinding;
        init();
    }

    void Texture::init()
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::CommandPool& commandPool = system::Executives::getInstance()->getPool();
        vk::ImageCreateInfo info = imageInfo;
        if(logicalDevice.createImage(&info, nullptr, &image) != vk::Result::eSuccess) throw std::runtime_error("Failed to create image!\n");
        vk::MemoryRequirements memoryRequirements;
        logicalDevice.getImageMemoryRequirements(image, &memoryRequirements);
        system::MemoryAllocationInfo allocationInfo;
        allocationInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        allocationInfo.memoryTypeBits = memoryRequirements.memoryTypeBits;
        allocationInfo.size = memoryRequirements.size;
        allocationInfo.alignment = memoryRequirements.alignment;
        memoryData = system::MemoryManager::getInstance()->allocateMemoryLazy(allocationInfo);

        vk::FenceCreateInfo fenceInfo;
        if(logicalDevice.createFence(&fenceInfo, nullptr, &textureReadyFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");

        vk::SemaphoreCreateInfo semaphoreInfo;
        if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &textureReadySemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");
        
        vk::CommandBufferAllocateInfo commandInfo;
        commandInfo.setCommandBufferCount(1);
        commandInfo.setCommandPool(commandPool);
        commandInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        if(logicalDevice.allocateCommandBuffers(&commandInfo, &updateCommandBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate command buffer!\n");
    }

    const vk::ImageView& Texture::getImageView() const 
    {
        return view;
    }

    vk::ImageView& Texture::getImageView()
    {
        return view;
    }

    void Texture::bindMemory()
    {
        const vk::DeviceMemory& memory = system::MemoryManager::getInstance()->getMemory(memoryData.index);
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::Queue& graphicsQueue = system::Executives::getInstance()->getGraphicsQueue();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(0);
        submitInfo.setPCommandBuffers(nullptr);
        submitInfo.setSignalSemaphoreCount(1);
        submitInfo.setPSignalSemaphores(&textureReadySemaphore);
        vk::PipelineStageFlags dstStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
        submitInfo.setPWaitDstStageMask(&dstStageFlags);
        graphicsQueue.submit(1, &submitInfo, textureReadyFence);

        if(logicalDevice.bindImageMemory(image, memory, memoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory to the texture!\n");

        vk::ImageSubresourceRange range;
        range.setAspectMask(vk::ImageAspectFlagBits::eColor);
        range.setLayerCount(1);
        range.setBaseArrayLayer(0);
        range.setLevelCount(1);
        range.setBaseMipLevel(0);

        vk::ImageViewCreateInfo viewInfo;
        viewInfo.setImage(image);
        viewInfo.setViewType(vk::ImageViewType::e2D);
        viewInfo.setFormat(imageInfo.format);
        viewInfo.setComponents(vk::ComponentMapping());
        viewInfo.setSubresourceRange(range);
        if(logicalDevice.createImageView(&viewInfo, nullptr, &view) != vk::Result::eSuccess) throw std::runtime_error("Failed to create image view!\n");
    }

    void Texture::update(const void* rawData)
    {
        memcpy(rawImageData.data(), rawData, rawImageData.size());
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        vk::Buffer transmissionBuffer;
        vk::BufferCreateInfo transmissionBufferInfo;
        transmissionBufferInfo.setQueueFamilyIndexCount(1);
        uint32_t index = system::Executives::getInstance()->getGraphicsQueueFamilyIndex();
        transmissionBufferInfo.setPQueueFamilyIndices(&index);
        transmissionBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        transmissionBufferInfo.setSize(imageInfo.extent.width * imageInfo.extent.height * imageInfo.channelCount);
        transmissionBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
        if(logicalDevice.createBuffer(&transmissionBufferInfo, nullptr, &transmissionBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create staging buffer!\n");

        vk::MemoryRequirements transmissionBufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(transmissionBuffer, &transmissionBufferMemoryRequirements);
        system::MemoryAllocationInfo allocInfo;
        allocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;         // TODO: make memory property non-coherent
        allocInfo.memoryTypeBits = transmissionBufferMemoryRequirements.memoryTypeBits;
        allocInfo.size = transmissionBufferMemoryRequirements.size;
        allocInfo.alignment = transmissionBufferMemoryRequirements.alignment;
        system::AllocatedMemoryData bufferData = system::MemoryManager::getInstance()->allocateMemory(allocInfo);
        vk::DeviceMemory& bufferMemory = system::MemoryManager::getInstance()->getMemory(bufferData.index);

        if(logicalDevice.bindBufferMemory(transmissionBuffer, bufferMemory, bufferData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory!\n");
        void * mappedMemory;
        if(logicalDevice.mapMemory(bufferMemory, bufferData.offset, transmissionBufferInfo.size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
        memcpy(mappedMemory, rawImageData.data(), transmissionBufferInfo.size);
        logicalDevice.unmapMemory(bufferMemory);

        logicalDevice.waitForFences(1, &textureReadyFence, true, ~0U);              //  move the sync operations out of here (if it is needed)
        if(logicalDevice.resetFences(1, &textureReadyFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset fence!\n");

        if(updateCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset buffer!\n");

        vk::CommandBufferBeginInfo commandBufferInfo;
        commandBufferInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        if(updateCommandBuffer.begin(&commandBufferInfo) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");

        vk::ImageSubresourceRange subresourceRange;
        subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
        subresourceRange.setLayerCount(1);
        subresourceRange.setBaseArrayLayer(0);
        subresourceRange.setLevelCount(1);
        subresourceRange.setBaseMipLevel(0);

        vk::ImageMemoryBarrier imageInitialBarrier;
        imageInitialBarrier.setSrcAccessMask(vk::AccessFlags());
        imageInitialBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        imageInitialBarrier.setImage(image);
        imageInitialBarrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        imageInitialBarrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        imageInitialBarrier.setOldLayout(imageInfo.layout);
        imageInitialBarrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
        imageInfo.layout = vk::ImageLayout::eTransferDstOptimal;
        imageInitialBarrier.setSubresourceRange(subresourceRange);
        updateCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), 
            0, nullptr,
            0, nullptr,
            1, &imageInitialBarrier);

        vk::ImageSubresourceLayers subresource;
        subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        subresource.setBaseArrayLayer(0);
        subresource.setLayerCount(1);
        subresource.setMipLevel(0);

        vk::BufferImageCopy copyInfo;
        copyInfo.setBufferOffset(0);
        copyInfo.setBufferImageHeight(0);
        copyInfo.setBufferRowLength(0);
        copyInfo.setImageExtent(imageInfo.extent);
        copyInfo.setImageOffset(vk::Offset3D());
        copyInfo.setImageSubresource(subresource);
        updateCommandBuffer.copyBufferToImage(transmissionBuffer, image, imageInfo.layout, 1, &copyInfo);

        vk::ImageMemoryBarrier imageBarrier;
        imageBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        imageBarrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        imageBarrier.setImage(image);
        imageBarrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        imageBarrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        imageBarrier.setOldLayout(imageInfo.layout);
        imageBarrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        imageInfo.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageBarrier.setSubresourceRange(subresourceRange);
        updateCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexShader, vk::DependencyFlags(), 
            0, nullptr,
            0, nullptr,
            1, &imageBarrier);
        
        updateCommandBuffer.end();

        const vk::Queue& graphicsQueue = system::Executives::getInstance()->getGraphicsQueue();
        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&updateCommandBuffer);
        submitInfo.setSignalSemaphoreCount(1);                  // recheck later
        submitInfo.setPSignalSemaphores(&textureReadySemaphore);               // too
        submitInfo.setWaitSemaphoreCount(1);                    // too
        submitInfo.setPWaitSemaphores(&textureReadySemaphore);                 // too
        vk::PipelineStageFlags dstStageFlags = vk::PipelineStageFlagBits::eVertexShader;
        submitInfo.setPWaitDstStageMask(&dstStageFlags);

        graphicsQueue.submit(1, &submitInfo, textureReadyFence);

        logicalDevice.waitForFences(1, &textureReadyFence, true, ~0U);              //  move the sync operations out of here (if it is needed)

        logicalDevice.destroyBuffer(transmissionBuffer, nullptr);
        system::MemoryManager::getInstance()->freeMemory(bufferData.index);
    }

    void Texture::destroy()
    {
        if(!transferred)                                                                        // If the texture wasn't moved to another texture..
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            if(textureReadyFence.operator VkFence() != VK_NULL_HANDLE)                          // ..and it was created properly, delete it
            {
                logicalDevice.destroyFence(textureReadyFence, nullptr);
                textureReadyFence = VkFence(0);
                logicalDevice.destroySemaphore(textureReadySemaphore, nullptr);
                textureReadySemaphore = VkSemaphore(0);
                logicalDevice.destroyImageView(view, nullptr);
                view = VkImageView(0);
                logicalDevice.destroyImage(image, nullptr);
                image = VkImage(0);
                system::MemoryManager::getInstance()->freeMemory(memoryData.index);
            }
        }
    }

    Texture::~Texture()
    {
        destroy();
    }

}