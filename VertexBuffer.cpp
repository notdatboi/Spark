#include"VertexBuffer.hpp"

namespace spk
{
    uint32_t VertexBuffer::count = 0;

    VertexBuffer::VertexBuffer(): identifier(count) {++count;}

    VertexBuffer::VertexBuffer(const VertexAlignmentInfo& cAlignmentInfo, const uint32_t cVertexBufferSize, const uint32_t cIndexBufferSize): 
        alignmentInfo(cAlignmentInfo), 
        vertexBufferSize(cVertexBufferSize), 
        indexBufferSize(cIndexBufferSize),
        identifier(count)
    {
        init();
        ++count;
    }

    VertexBuffer::VertexBuffer(const VertexBuffer& vb): identifier(count)
    {
        ++count;
        create(vb.alignmentInfo, vb.vertexBufferSize, vb.indexBufferSize);
    }

    VertexBuffer::VertexBuffer(VertexBuffer&& vb): identifier(vb.identifier)
    {
        vb.transferred = true;
        alignmentInfo = std::move(vb.alignmentInfo);
        vertexBufferSize = std::move(vb.vertexBufferSize);
        indexBufferSize = std::move(vb.indexBufferSize);
        vertexMemoryData = std::move(vb.vertexMemoryData);
        indexMemoryData = std::move(vb.indexMemoryData);
        vertexBuffer = std::move(vb.vertexBuffer);
        indexBuffer = std::move(vb.indexBuffer);
        vertexBufferUpdatedFence = std::move(vb.vertexBufferUpdatedFence);
        vertexBufferUpdatedSemaphore = std::move(vb.vertexBufferUpdatedSemaphore);
        indexBufferUpdatedFence = std::move(vb.indexBufferUpdatedFence);
        indexBufferUpdatedSemaphore = std::move(vb.indexBufferUpdatedSemaphore);
    }

    void VertexBuffer::create(const VertexAlignmentInfo& cAlignmentInfo, const uint32_t cVertexBufferSize, const uint32_t cIndexBufferSize)
    {
        identifier = count;
        ++count;
        alignmentInfo = cAlignmentInfo;
        vertexBufferSize = cVertexBufferSize;
        indexBufferSize = cIndexBufferSize;
        init();
    }

    const uint32_t VertexBuffer::getIdentifier() const
    {
        return identifier;
    }

    const vk::Buffer& VertexBuffer::getVertexBuffer() const 
    {
        return vertexBuffer;
    }

    const vk::Buffer& VertexBuffer::getIndexBuffer() const
    {
        return indexBuffer;
    }

    const uint32_t VertexBuffer::getVertexBufferSize() const
    {
        return vertexBufferSize;
    }

    const uint32_t VertexBuffer::getIndexBufferSize() const
    {
        return indexBufferSize;
    }

    const vk::Fence* VertexBuffer::getIndexBufferFence() const
    {
        return &indexBufferUpdatedFence;
    }

    const vk::Fence* VertexBuffer::getVertexBufferFence() const
    {
        return &vertexBufferUpdatedFence;
    }

    const vk::Semaphore* VertexBuffer::getIndexBufferSemaphore() const
    {
        return &indexBufferUpdatedSemaphore;
    }

    const vk::Semaphore* VertexBuffer::getVertexBufferSemaphore() const
    {
        return &vertexBufferUpdatedSemaphore;
    }

    const VertexAlignmentInfo& VertexBuffer::getAlignmentInfo() const
    {
        return alignmentInfo;
    }

    VertexBuffer& VertexBuffer::operator=(const VertexBuffer& rBuffer)
    {
        destroy();
        create(rBuffer.alignmentInfo, rBuffer.vertexBufferSize, rBuffer.indexBufferSize);
        identifier = count;
        ++count;
        return *this;
    }

    VertexBuffer& VertexBuffer::operator=(VertexBuffer& rBuffer)
    {
        destroy();
        create(rBuffer.alignmentInfo, rBuffer.vertexBufferSize, rBuffer.indexBufferSize);
        identifier = count;
        ++count;
        return *this;
    }

    VertexBuffer& VertexBuffer::operator=(VertexBuffer&& rBuffer)
    {
        destroy();
        identifier = rBuffer.identifier;
        rBuffer.transferred = true;
        alignmentInfo = std::move(rBuffer.alignmentInfo);
        vertexBufferSize = std::move(rBuffer.vertexBufferSize);
        indexBufferSize = std::move(rBuffer.indexBufferSize);
        vertexMemoryData = std::move(rBuffer.vertexMemoryData);
        indexMemoryData = std::move(rBuffer.indexMemoryData);
        vertexBuffer = std::move(rBuffer.vertexBuffer);
        indexBuffer = std::move(rBuffer.indexBuffer);
        vertexBufferUpdatedFence = std::move(rBuffer.vertexBufferUpdatedFence);
        vertexBufferUpdatedSemaphore = std::move(rBuffer.vertexBufferUpdatedSemaphore);
        indexBufferUpdatedFence = std::move(rBuffer.indexBufferUpdatedFence);
        indexBufferUpdatedSemaphore = std::move(rBuffer.indexBufferUpdatedSemaphore);
        return *this;
    }

    void VertexBuffer::init()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        const vk::CommandPool& commandPool = Executives::getInstance()->getPool();

        vk::BufferCreateInfo vBufferInfo;
        vBufferInfo.setSize(vertexBufferSize);
        vBufferInfo.setUsage(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
        vBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        vBufferInfo.setQueueFamilyIndexCount(1);
        uint32_t queueFamIndex = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        vBufferInfo.setPQueueFamilyIndices(&queueFamIndex);
        if(logicalDevice.createBuffer(&vBufferInfo, nullptr, &vertexBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create buffer!\n");

        vk::MemoryRequirements vBufferMemoryReq;
        logicalDevice.getBufferMemoryRequirements(vertexBuffer, &vBufferMemoryReq);
        MemoryAllocationInfo vAllocInfo;
        vAllocInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vAllocInfo.size = vBufferMemoryReq.size;
        vAllocInfo.memoryTypeBits = vBufferMemoryReq.memoryTypeBits;
        vAllocInfo.alignment = vBufferMemoryReq.alignment;
        vertexMemoryData = MemoryManager::getInstance()->allocateMemoryLazy(vAllocInfo);

        vk::FenceCreateInfo fenceInfo;
        if(logicalDevice.createFence(&fenceInfo, nullptr, &vertexBufferUpdatedFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");
        vk::SemaphoreCreateInfo semaphoreInfo;
        if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &vertexBufferUpdatedSemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");

        vk::CommandBufferAllocateInfo commandInfo;
        commandInfo.setCommandBufferCount(1);
        commandInfo.setCommandPool(commandPool);
        commandInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        if(logicalDevice.allocateCommandBuffers(&commandInfo, &vertexUpdateCommandBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate command buffer!\n");

        if(indexBufferSize != 0)
        {
            vk::BufferCreateInfo iBufferInfo;
            iBufferInfo.setSize(indexBufferSize);
            iBufferInfo.setUsage(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
            iBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
            iBufferInfo.setQueueFamilyIndexCount(1);
            iBufferInfo.setPQueueFamilyIndices(&queueFamIndex);
            if(logicalDevice.createBuffer(&iBufferInfo, nullptr, &indexBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create buffer!\n");

            vk::MemoryRequirements iBufferMemoryReq;
            logicalDevice.getBufferMemoryRequirements(indexBuffer, &iBufferMemoryReq);
            MemoryAllocationInfo iAllocInfo;
            iAllocInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
            iAllocInfo.size = iBufferMemoryReq.size;
            iAllocInfo.alignment = iBufferMemoryReq.alignment;
            iAllocInfo.memoryTypeBits = iBufferMemoryReq.memoryTypeBits;
            indexMemoryData = MemoryManager::getInstance()->allocateMemoryLazy(iAllocInfo);

            if(logicalDevice.createFence(&fenceInfo, nullptr, &indexBufferUpdatedFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");
            if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &indexBufferUpdatedSemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");

            if(logicalDevice.allocateCommandBuffers(&commandInfo, &indexUpdateCommandBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate command buffer!\n");
        }
    }

    void VertexBuffer::updateVertexBuffer(const void* data)
    {
        update(data, true);
    }

    void VertexBuffer::updateIndexBuffer(const void* data)
    {
        if(indexBufferSize == 0) throw std::runtime_error("Trying to update empty index buffer.\n");
        update(data, false);
    }

    void VertexBuffer::update(const void* data, bool vertex)
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        const vk::Queue& graphicsQueue = Executives::getInstance()->getGraphicsQueue();
        const vk::CommandBuffer& updateCommandBuffer = vertex ? vertexUpdateCommandBuffer : indexUpdateCommandBuffer;

        vk::Buffer transmissionBuffer;
        vk::BufferCreateInfo transmissionBufferInfo;
        transmissionBufferInfo.setQueueFamilyIndexCount(1);
        uint32_t index = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        transmissionBufferInfo.setPQueueFamilyIndices(&index);
        transmissionBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        transmissionBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
        transmissionBufferInfo.setSize(vertex ? vertexBufferSize : indexBufferSize);
        if(logicalDevice.createBuffer(&transmissionBufferInfo, nullptr, &transmissionBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create staging buffer!\n");

        vk::MemoryRequirements transmissionBufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(transmissionBuffer, &transmissionBufferMemoryRequirements);
        MemoryAllocationInfo allocInfo;
        allocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;         // TODO: also make memory property non-coherent
        allocInfo.memoryTypeBits = transmissionBufferMemoryRequirements.memoryTypeBits;
        allocInfo.size = transmissionBufferMemoryRequirements.size;
        allocInfo.alignment = transmissionBufferMemoryRequirements.alignment;
        AllocatedMemoryData transmissionBufferData = MemoryManager::getInstance()->allocateMemory(allocInfo);
        const vk::DeviceMemory& transmissionBufferMemory = MemoryManager::getInstance()->getMemory(transmissionBufferData.index);

        if(logicalDevice.bindBufferMemory(transmissionBuffer, transmissionBufferMemory, transmissionBufferData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory!\n");
        void * mappedMemory;
        if(logicalDevice.mapMemory(transmissionBufferMemory, transmissionBufferData.offset, transmissionBufferInfo.size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
        memcpy(mappedMemory, data, vertex ? vertexBufferSize : indexBufferSize);
        logicalDevice.unmapMemory(transmissionBufferMemory);

        vk::CommandBufferBeginInfo commandBufferInfo;
        //commandBufferInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        if(updateCommandBuffer.begin(&commandBufferInfo) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");

        vk::BufferCopy transmissionCopyData;
        transmissionCopyData.setSrcOffset(transmissionBufferData.offset);
        transmissionCopyData.setDstOffset(vertex ? vertexMemoryData.offset : indexMemoryData.offset);
        transmissionCopyData.setSize(vertex ? vertexBufferSize : indexBufferSize);
        updateCommandBuffer.copyBuffer(transmissionBuffer, vertex ? vertexBuffer : indexBuffer, 1, &transmissionCopyData);

        updateCommandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&updateCommandBuffer);
        submitInfo.setSignalSemaphoreCount(1);                 
        submitInfo.setPSignalSemaphores(vertex ? &vertexBufferUpdatedSemaphore : &indexBufferUpdatedSemaphore);
        submitInfo.setWaitSemaphoreCount(0);                   
        submitInfo.setPWaitSemaphores(nullptr);                 
        vk::PipelineStageFlags dstStageFlags = vk::PipelineStageFlagBits::eVertexInput;
        submitInfo.setPWaitDstStageMask(&dstStageFlags);

        graphicsQueue.submit(1, &submitInfo, vertex ? vertexBufferUpdatedFence : indexBufferUpdatedFence);

        logicalDevice.waitForFences(1, vertex ? &vertexBufferUpdatedFence : &indexBufferUpdatedFence, true, ~0U);              //  move the sync operations out of here
        if(updateCommandBuffer.reset(vk::CommandBufferResetFlags()) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset buffer!\n");
        logicalDevice.destroyBuffer(transmissionBuffer, nullptr);
        MemoryManager::getInstance()->freeMemory(transmissionBufferData.index);
    }

    void VertexBuffer::bindMemory()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        const vk::DeviceMemory& vertexBufferMemory = MemoryManager::getInstance()->getMemory(vertexMemoryData.index);
        if(indexBufferSize != 0)
        {
            const vk::DeviceMemory& indexBufferMemory = MemoryManager::getInstance()->getMemory(indexMemoryData.index);
            if(logicalDevice.bindBufferMemory(indexBuffer, MemoryManager::getInstance()->getMemory(indexMemoryData.index), indexMemoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind buffer memory!\n");
        }

        if(logicalDevice.bindBufferMemory(vertexBuffer, MemoryManager::getInstance()->getMemory(vertexMemoryData.index), vertexMemoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind buffer memory!\n");
    }

    void VertexBuffer::destroy()
    {
        if(transferred) return;
        if(vertexBuffer.operator VkBuffer() == VK_NULL_HANDLE && indexBuffer.operator VkBuffer() == VK_NULL_HANDLE) return;
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();

        logicalDevice.destroyBuffer(vertexBuffer, nullptr);
        MemoryManager::getInstance()->freeMemory(vertexMemoryData.index);
        vertexBuffer = VkBuffer(0);
        logicalDevice.destroyFence(vertexBufferUpdatedFence, nullptr);
        vertexBufferUpdatedFence = VkFence(0);
        logicalDevice.destroySemaphore(vertexBufferUpdatedSemaphore, nullptr);
        vertexBufferUpdatedSemaphore = VkSemaphore(0);

        if(indexBufferSize != 0)
        {
            logicalDevice.destroyBuffer(indexBuffer, nullptr);
            MemoryManager::getInstance()->freeMemory(indexMemoryData.index);
            indexBuffer = VkBuffer(0);
            logicalDevice.destroyFence(indexBufferUpdatedFence, nullptr);
            indexBufferUpdatedFence = VkFence(0);
            logicalDevice.destroySemaphore(indexBufferUpdatedSemaphore, nullptr);
            indexBufferUpdatedSemaphore = VkSemaphore(0);
        }
    }

    VertexBuffer::~VertexBuffer()
    {
        destroy();
    }

}