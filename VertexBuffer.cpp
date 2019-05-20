#include"VertexBuffer.hpp"

namespace spk
{

    VertexBuffer::VertexBuffer(){}

    VertexBuffer::VertexBuffer(const VertexAlignmentInfo& cAlignmentInfo, const uint32_t cSize): alignmentInfo(cAlignmentInfo), size(cSize)
    {
        init();
    }

    VertexBuffer::VertexBuffer(const VertexBuffer& vb)
    {
        create(vb.alignmentInfo, vb.size);
    }

    VertexBuffer::VertexBuffer(VertexBuffer&& vb)
    {
        vb.transferred = true;
        alignmentInfo = std::move(vb.alignmentInfo);
        size = std::move(vb.size);
        memoryData = std::move(vb.memoryData);
        buffer = std::move(vb.buffer);
        bufferUpdatedFence = std::move(vb.bufferUpdatedFence);
        bufferUpdatedSemaphore = std::move(vb.bufferUpdatedSemaphore);
    }

    void VertexBuffer::create(const VertexAlignmentInfo& cAlignmentInfo, const uint32_t cSize)
    {
        alignmentInfo = cAlignmentInfo;
        size = cSize;
        init();
    }

    VertexBuffer& VertexBuffer::operator=(const VertexBuffer& rBuffer)
    {
        destroy();
        create(rBuffer.alignmentInfo, rBuffer.size);
        return *this;
    }

    VertexBuffer& VertexBuffer::operator=(VertexBuffer& rBuffer)
    {
        destroy();
        create(rBuffer.alignmentInfo, rBuffer.size);
        return *this;
    }

    VertexBuffer& VertexBuffer::operator=(VertexBuffer&& rBuffer)
    {
        destroy();
        rBuffer.transferred = true;
        alignmentInfo = std::move(rBuffer.alignmentInfo);
        size = std::move(rBuffer.size);
        memoryData = std::move(rBuffer.memoryData);
        buffer = std::move(rBuffer.buffer);
        bufferUpdatedFence = std::move(rBuffer.bufferUpdatedFence);
        bufferUpdatedSemaphore = std::move(rBuffer.bufferUpdatedSemaphore);
        return *this;
    }

    void VertexBuffer::init()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.setSize(size);
        bufferInfo.setUsage(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
        bufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        bufferInfo.setQueueFamilyIndexCount(1);
        uint32_t queueFamIndex = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        bufferInfo.setPQueueFamilyIndices(&queueFamIndex);
        if(logicalDevice.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create buffer!\n");

        vk::MemoryRequirements bufferMemoryReq;
        logicalDevice.getBufferMemoryRequirements(buffer, &bufferMemoryReq);
        MemoryAllocationInfo allocInfo;
        allocInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        allocInfo.size = bufferMemoryReq.size;
        allocInfo.memoryTypeBits = bufferMemoryReq.memoryTypeBits;
        memoryData = MemoryManager::getInstance()->allocateMemoryLazy(allocInfo);

        vk::FenceCreateInfo fenceInfo;
        if(logicalDevice.createFence(&fenceInfo, nullptr, &bufferUpdatedFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");

        vk::SemaphoreCreateInfo semaphoreInfo;
        if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &bufferUpdatedSemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");
    }

    void VertexBuffer::update(const vk::CommandBuffer& updateCommandBuffer, const void* data)
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        const vk::Queue& graphicsQueue = Executives::getInstance()->getGraphicsQueue();

        vk::Buffer transmissionBuffer;
        vk::BufferCreateInfo transmissionBufferInfo;
        transmissionBufferInfo.setQueueFamilyIndexCount(1);
        uint32_t index = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        transmissionBufferInfo.setPQueueFamilyIndices(&index);
        transmissionBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        transmissionBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
        transmissionBufferInfo.setSize(size);
        if(logicalDevice.createBuffer(&transmissionBufferInfo, nullptr, &transmissionBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create staging buffer!\n");

        vk::MemoryRequirements transmissionBufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(transmissionBuffer, &transmissionBufferMemoryRequirements);
        MemoryAllocationInfo allocInfo;
        allocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;         // TODO: also make memory property non-coherent
        allocInfo.memoryTypeBits = transmissionBufferMemoryRequirements.memoryTypeBits;
        allocInfo.size = transmissionBufferMemoryRequirements.size;
        AllocatedMemoryData transmissionBufferData = MemoryManager::getInstance()->allocateMemory(allocInfo);
        vk::DeviceMemory& transmissionBufferMemory = MemoryManager::getInstance()->getMemory(transmissionBufferData.index);

        if(logicalDevice.bindBufferMemory(transmissionBuffer, transmissionBufferMemory, transmissionBufferData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory!\n");
        void * mappedMemory;
        if(logicalDevice.mapMemory(transmissionBufferMemory, transmissionBufferData.offset, transmissionBufferInfo.size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
        memcpy(mappedMemory, data, size);
        logicalDevice.unmapMemory(transmissionBufferMemory);

        vk::CommandBufferBeginInfo commandBufferInfo;
        commandBufferInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        if(updateCommandBuffer.begin(&commandBufferInfo) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");

        vk::BufferCopy transmissionCopyData;
        transmissionCopyData.setSrcOffset(transmissionBufferData.offset);
        transmissionCopyData.setDstOffset(memoryData.offset);
        transmissionCopyData.setSize(size);
        updateCommandBuffer.copyBuffer(transmissionBuffer, buffer, 1, &transmissionCopyData);

        updateCommandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&updateCommandBuffer);
        submitInfo.setSignalSemaphoreCount(1);                 
        submitInfo.setPSignalSemaphores(&bufferUpdatedSemaphore);               
        submitInfo.setWaitSemaphoreCount(0);                   
        submitInfo.setPWaitSemaphores(nullptr);                 
        vk::PipelineStageFlags dstStageFlags = vk::PipelineStageFlagBits::eVertexInput;
        submitInfo.setPWaitDstStageMask(&dstStageFlags);

        graphicsQueue.submit(1, &submitInfo, bufferUpdatedFence);

        logicalDevice.waitForFences(1, &bufferUpdatedFence, true, ~0U);              //  move the sync operations out of here
        if(updateCommandBuffer.reset(vk::CommandBufferResetFlags()) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset buffer!\n");
        MemoryManager::getInstance()->freeMemory(transmissionBufferData.index);
        logicalDevice.destroyBuffer(transmissionBuffer, nullptr);
    }

    void VertexBuffer::bindMemory()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        const vk::DeviceMemory& vertexBufferMemory = MemoryManager::getInstance()->getMemory(memoryData.index);

        if(logicalDevice.bindBufferMemory(buffer, MemoryManager::getInstance()->getMemory(memoryData.index), memoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind buffer memory!\n");
    }

    void VertexBuffer::destroy()
    {
        if(transferred) return;
        if(buffer.operator VkBuffer() == VK_NULL_HANDLE) return;
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();

        MemoryManager::getInstance()->freeMemory(memoryData.index);
        logicalDevice.destroyBuffer(buffer, nullptr);
        buffer = VkBuffer(0);
        logicalDevice.destroyFence(bufferUpdatedFence, nullptr);
        bufferUpdatedFence = VkFence(0);
        logicalDevice.destroySemaphore(bufferUpdatedSemaphore, nullptr);
        bufferUpdatedSemaphore = VkSemaphore(0);
    }

    VertexBuffer::~VertexBuffer()
    {
        destroy();
    }

}