#include"Texture.hpp"

namespace spk
{

    ImageData::ImageData()
    {
        type = vk::ImageType::e2D;
        format = vk::Format::eR8G8B8A8Unorm;
        mipLevels = 1;
        arrayLayers = 1;
        samples = vk::SampleCountFlagBits::e1;
        tiling = vk::ImageTiling::eOptimal;
        usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
        uint32_t presentFamilyIndex = Executives::getInstance()->getPresentQueueFamilyIndex();
        uint32_t graphicsFamilyIndex = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        if(presentFamilyIndex == graphicsFamilyIndex)
        {
            sharingMode = vk::SharingMode::eExclusive;
            queueFamilyIndexCount = 1;
            queueFamilyIndices.push_back(graphicsFamilyIndex);
        }
        else
        {
            sharingMode = vk::SharingMode::eConcurrent;
            queueFamilyIndexCount = 2;
            queueFamilyIndices.push_back(graphicsFamilyIndex);
            queueFamilyIndices.push_back(presentFamilyIndex);
        }
        layout = vk::ImageLayout::eUndefined;
        channelCount = 4;
    }

    ImageData::operator vk::ImageCreateInfo()
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

    Texture::Texture(){}

    Texture::Texture(const uint32_t width, const uint32_t height, const void * rawData)
    {
        create(width, height, rawData);
    }

    void Texture::create(const uint32_t width, const uint32_t height, const void * rawData)
    {
        imageData.extent = vk::Extent3D(width, height, 1);
        rawImageData = rawData;
        init();
    }

    void Texture::init()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        vk::ImageCreateInfo info = imageData;
        if(logicalDevice.createImage(&info, nullptr, &image) != vk::Result::eSuccess) throw std::runtime_error("Failed to create image!\n");
        vk::MemoryRequirements memoryRequirements;
        logicalDevice.getImageMemoryRequirements(image, &memoryRequirements);
        MemoryAllocationInfo allocationInfo;
        allocationInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        allocationInfo.memoryTypeBits = memoryRequirements.memoryTypeBits;
        allocationInfo.size = memoryRequirements.size;
        memoryData = MemoryManager::getInstance()->allocateMemoryLazy(allocationInfo);

        vk::FenceCreateInfo fenceInfo;
        if(logicalDevice.createFence(&fenceInfo, nullptr, &textureReadyFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");

        vk::SemaphoreCreateInfo semaphoreInfo;
        if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &textureReadySemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");
    }

    const vk::ImageView& Texture::getImageView() const 
    {
        return view;
    }

    vk::ImageView& Texture::getImageView()
    {
        return view;
    }

    void Texture::bindMemory(const vk::CommandBuffer& memoryBindBuffer)
    {
        const vk::DeviceMemory& memory = MemoryManager::getInstance()->getMemory(memoryData.index);
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        if(logicalDevice.bindImageMemory(image, memory, memoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory to the texture!\n");

        vk::Buffer transmissionBuffer;
        vk::BufferCreateInfo transmissionBufferInfo;
        transmissionBufferInfo.setQueueFamilyIndexCount(1);
        uint32_t index = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        transmissionBufferInfo.setPQueueFamilyIndices(&index);
        transmissionBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        transmissionBufferInfo.setSize(imageData.extent.width * imageData.extent.height * imageData.channelCount);
        transmissionBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);             // add transferDst?
        if(logicalDevice.createBuffer(&transmissionBufferInfo, nullptr, &transmissionBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create staging buffer!\n");

        vk::MemoryRequirements transmissionBufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(transmissionBuffer, &transmissionBufferMemoryRequirements);
        MemoryAllocationInfo allocInfo;
        allocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;         // make memory property non-coherent
        allocInfo.memoryTypeBits = transmissionBufferMemoryRequirements.memoryTypeBits;
        allocInfo.size = transmissionBufferMemoryRequirements.size;
        AllocatedMemoryData bufferData = MemoryManager::getInstance()->allocateMemory(allocInfo);
        vk::DeviceMemory& bufferMemory = MemoryManager::getInstance()->getMemory(bufferData.index);

        void * mappedMemory;
        if(logicalDevice.mapMemory(bufferMemory, bufferData.offset, transmissionBufferInfo.size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
        memcpy(mappedMemory, rawImageData, transmissionBufferInfo.size);
        logicalDevice.unmapMemory(bufferMemory);
        if(logicalDevice.bindBufferMemory(transmissionBuffer, bufferMemory, bufferData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory!\n");

        vk::CommandBufferBeginInfo commandBufferInfo;
        commandBufferInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        if(memoryBindBuffer.begin(&commandBufferInfo) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");

        /* EXPERIMENTAL */

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
        imageInitialBarrier.setOldLayout(imageData.layout);
        imageInitialBarrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
        imageData.layout = vk::ImageLayout::eTransferDstOptimal;
        imageInitialBarrier.setSubresourceRange(subresourceRange);
        memoryBindBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), 
            0, nullptr,
            0, nullptr,
            1, &imageInitialBarrier);
        
        /* */

        vk::ImageSubresourceLayers subresource;
        subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        subresource.setBaseArrayLayer(0);
        subresource.setLayerCount(1);
        subresource.setMipLevel(0);

        vk::BufferImageCopy copyInfo;
        copyInfo.setBufferOffset(0);
        copyInfo.setBufferImageHeight(0);
        std::cout << transmissionBufferInfo.size << '\n';
        copyInfo.setBufferRowLength(0);
        copyInfo.setImageExtent(imageData.extent);
        copyInfo.setImageOffset(vk::Offset3D());
        copyInfo.setImageSubresource(subresource);
        memoryBindBuffer.copyBufferToImage(transmissionBuffer, image, imageData.layout, 1, &copyInfo);

        /* EXPERIMENTAL */

        vk::ImageMemoryBarrier imageBarrier;
        imageBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        imageBarrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        imageBarrier.setImage(image);
        imageBarrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        imageBarrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        imageBarrier.setOldLayout(imageData.layout);
        imageBarrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        imageData.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageBarrier.setSubresourceRange(subresourceRange);
        memoryBindBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexShader, vk::DependencyFlags(), 
            0, nullptr,
            0, nullptr,
            1, &imageBarrier);
        
        /* */

        memoryBindBuffer.end();

        const vk::Queue& graphicsQueue = Executives::getInstance()->getGraphicsQueue();
        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&memoryBindBuffer);
        submitInfo.setSignalSemaphoreCount(1);                  // recheck later
        submitInfo.setPSignalSemaphores(&textureReadySemaphore);               // too
        submitInfo.setWaitSemaphoreCount(0);                    // too
        submitInfo.setPWaitSemaphores(nullptr);                 // too
        vk::PipelineStageFlags dstStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
        submitInfo.setPWaitDstStageMask(&dstStageFlags);

        graphicsQueue.submit(1, &submitInfo, textureReadyFence);

        logicalDevice.waitForFences(1, &textureReadyFence, true, ~0U);
        if(memoryBindBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset buffer!\n");
        MemoryManager::getInstance()->freeMemory(bufferData.index);
        logicalDevice.destroyBuffer(transmissionBuffer, nullptr);

        vk::ImageSubresourceRange range;
        range.setAspectMask(vk::ImageAspectFlagBits::eColor);
        range.setLayerCount(1);
        range.setBaseArrayLayer(0);
        range.setLevelCount(1);
        range.setBaseMipLevel(0);

        vk::ImageViewCreateInfo viewInfo;
        viewInfo.setImage(image);
        viewInfo.setViewType(vk::ImageViewType::e2D);
        viewInfo.setFormat(imageData.format);
        viewInfo.setComponents(vk::ComponentMapping());
        viewInfo.setSubresourceRange(range);
        if(logicalDevice.createImageView(&viewInfo, nullptr, &view) != vk::Result::eSuccess) throw std::runtime_error("Failed to create image view!\n");
    }

    Texture::~Texture()
    {
        MemoryManager::getInstance()->freeMemory(memoryData.index);
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        logicalDevice.destroyFence(textureReadyFence, nullptr);
        logicalDevice.destroyImage(image, nullptr);
        logicalDevice.destroyImageView(view, nullptr);
    }

}