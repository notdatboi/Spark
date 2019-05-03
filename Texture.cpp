#include"Texture.hpp"

namespace spk
{

    ImageData::ImageData()
    {
        type = vk::ImageType::e2D;
        format = vk::Format::eR8G8B8A8Uint;
        mipLevels = 0;
        arrayLayers = 0;
        samples = vk::SampleCountFlagBits::e1;
        tiling = vk::ImageTiling::eOptimal;
        usage = vk::ImageUsageFlagBits::eColorAttachment;
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
        layout = vk::ImageLayout::eTransferDstOptimal;
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

    Texture::Texture(const uint32_t width, const uint32_t height, const void * rawData)
    {
        imageData.extent = vk::Extent3D(width, height);
        rawImageData = rawData;
        create();
    }

    void Texture::create()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        vk::ImageCreateInfo info = imageData;
        logicalDevice.createImage(&info, nullptr, &image);
        vk::MemoryRequirements memoryRequirements;
        logicalDevice.getImageMemoryRequirements(image, &memoryRequirements);
        MemoryAllocationInfo allocationInfo;
        allocationInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        allocationInfo.memoryTypeBits = memoryRequirements.memoryTypeBits;
        allocationInfo.size = memoryRequirements.size;
        memoryData = MemoryManager::getInstance()->allocateMemoryLazy(allocationInfo);
        
        vk::FenceCreateInfo fenceInfo;
        logicalDevice.createFence(&fenceInfo, nullptr, &textureReadyFence);
    }

    const vk::Image& Texture::getImage() const 
    {
        return image;
    }

    vk::Image& Texture::getImage()
    {
        return image;
    }

    void Texture::bindMemory(const vk::CommandBuffer& memoryBindBuffer)
    {
        const vk::DeviceMemory& memory = MemoryManager::getInstance()->getMemory(memoryData.index);
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        logicalDevice.bindImageMemory(image, memory, memoryData.offset);

        vk::Buffer transmissionBuffer;
        vk::BufferCreateInfo transmissionBufferInfo;
        transmissionBufferInfo.setQueueFamilyIndexCount(1);
        uint32_t index = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        transmissionBufferInfo.setPQueueFamilyIndices(&index);
        transmissionBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        transmissionBufferInfo.setSize(imageData.extent.width * imageData.extent.height);
        transmissionBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);             // add transferDst?
        logicalDevice.createBuffer(&transmissionBufferInfo, nullptr, &transmissionBuffer);

        vk::MemoryRequirements transmissionBufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(transmissionBuffer, &transmissionBufferMemoryRequirements);
        MemoryAllocationInfo allocInfo;
        allocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;         // make memory property non-coherent
        allocInfo.memoryTypeBits = transmissionBufferMemoryRequirements.memoryTypeBits;
        allocInfo.size = transmissionBufferMemoryRequirements.size;
        AllocatedMemoryData bufferData = MemoryManager::getInstance()->allocateMemory(allocInfo);
        vk::DeviceMemory& memory = MemoryManager::getInstance()->getMemory(bufferData.index);

        void * mappedMemory;
        if(logicalDevice.mapMemory(memory, bufferData.offset, transmissionBufferInfo.size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
        memcpy(mappedMemory, rawImageData, transmissionBufferInfo.size);
        logicalDevice.unmapMemory(memory);
        if(logicalDevice.bindBufferMemory(transmissionBuffer, memory, bufferData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory!\n");

        vk::CommandBufferBeginInfo commandBufferInfo;
        commandBufferInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        memoryBindBuffer.begin(&commandBufferInfo);

        vk::ImageSubresourceLayers subresource;
        subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        subresource.setBaseArrayLayer(0);
        subresource.setLayerCount(1);
        subresource.setMipLevel(1);

        vk::BufferImageCopy copyInfo;
        copyInfo.setBufferOffset(bufferData.offset);
        copyInfo.setBufferImageHeight(imageData.extent.height);
        copyInfo.setBufferRowLength(imageData.extent.width);
        copyInfo.setImageExtent(imageData.extent);
        copyInfo.setImageOffset(vk::Offset3D());
        copyInfo.setImageSubresource(subresource);
        memoryBindBuffer.copyBufferToImage(transmissionBuffer, image, imageData.layout, 1, &copyInfo);

        memoryBindBuffer.end();

        const vk::Queue& graphicsQueue = Executives::getInstance()->getGraphicsQueue();
        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&memoryBindBuffer);
        submitInfo.setSignalSemaphoreCount(0);                  // recheck later
        submitInfo.setPSignalSemaphores(nullptr);               // too
        submitInfo.setWaitSemaphoreCount(0);                    // too
        submitInfo.setPWaitSemaphores(nullptr);                 // too
        vk::PipelineStageFlags dstStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
        submitInfo.setPWaitDstStageMask(&dstStageFlags);

        graphicsQueue.submit(1, &submitInfo, textureReadyFence);

        logicalDevice.waitForFences(1, &textureReadyFence, true, ~0U);
        MemoryManager::getInstance()->freeMemory(bufferData.index);
        logicalDevice.destroyBuffer(transmissionBuffer, nullptr);
    }

    Texture::~Texture()
    {
        MemoryManager::getInstance()->freeMemory(memoryData.index);
    }

}