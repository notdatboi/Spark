#include"VertexBuffer.hpp"

namespace spk
{
    uint32_t VertexBuffer::count = 0;

    VertexBuffer::VertexBuffer(): identifier(count) {++count;}

    VertexBuffer::VertexBuffer(const std::vector<VertexAlignmentInfo>& cAlignmentInfos, const std::vector<uint32_t>& cVertexBufferSizes, const uint32_t cIndexBufferSize): 
        alignmentInfos(cAlignmentInfos), 
        vertexBufferSizes(cVertexBufferSizes),
        indexBufferSize(cIndexBufferSize),
        identifier(count)
    {
        init();
        ++count;
    }

    VertexBuffer::VertexBuffer(const VertexBuffer& vb): identifier(count)
    {
        ++count;
        create(vb.alignmentInfos, vb.vertexBufferSizes, vb.indexBufferSize);
    }

    void VertexBuffer::create(const std::vector<VertexAlignmentInfo>& cAlignmentInfos, const std::vector<uint32_t>& cVertexBufferSizes, const uint32_t cIndexBufferSize)
    {
        identifier = count;
        ++count;
        alignmentInfos = cAlignmentInfos;
        vertexBufferSizes = cVertexBufferSizes;
        indexBufferSize = cIndexBufferSize;
        init();
    }

    const uint32_t VertexBuffer::getIdentifier() const
    {
        return identifier;
    }

    const vk::Buffer& VertexBuffer::getVertexBuffer(const uint32_t binding) const
    {
        return vertexBuffers.at(binding).buffer.getBuffer();
    }

    const vk::Buffer& VertexBuffer::getIndexBuffer() const
    {
        return indexBuffer.getBuffer();
    }

    const uint32_t VertexBuffer::getVertexBufferSize(const uint32_t binding) const
    {
        return vertexBuffers.at(binding).size;
    }

    const uint32_t VertexBuffer::getIndexBufferSize() const
    {
        return indexBufferSize;
    }

    const vk::Fence* VertexBuffer::getIndexBufferFence() const
    {
        return &indexBufferUpdatedFence;
    }

    const vk::Fence* VertexBuffer::getVertexBufferFence(const uint32_t binding) const
    {
        return &vertexBuffers.at(binding).updatedFence;
    }

    const vk::Semaphore* VertexBuffer::getIndexBufferSemaphore() const
    {
        return &indexBufferUpdatedSemaphore;
    }

    const vk::Semaphore* VertexBuffer::getVertexBufferSemaphore(const uint32_t binding) const
    {
        return &(vertexBuffers.at(binding).updatedSemaphore);
    }

    const std::vector<VertexAlignmentInfo>& VertexBuffer::getAlignmentInfos() const
    {
        return alignmentInfos;
    }

    void VertexBuffer::setInstancingOptions(const uint32_t count, const uint32_t first)
    {
        instanceCount = count;
        firstInstance = first;
    }

    const uint32_t VertexBuffer::getInstanceCount() const
    {
        return instanceCount;
    }

    const uint32_t VertexBuffer::getFirstInstance() const
    {
        return firstInstance;
    }

    VertexBuffer& VertexBuffer::operator=(const VertexBuffer& rBuffer)
    {
        destroy();
        create(rBuffer.alignmentInfos, rBuffer.vertexBufferSizes, rBuffer.indexBufferSize);
        identifier = count;
        ++count;
        return *this;
    }

    void VertexBuffer::init()
    {
        instanceCount = 1;
        firstInstance = 0;
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::CommandPool& commandPool = system::Executives::getInstance()->getPool();
        uint32_t queueFamIndex = system::Executives::getInstance()->getGraphicsQueueFamilyIndex();
        vk::FenceCreateInfo fenceInfo;
        vk::SemaphoreCreateInfo semaphoreInfo;
        vk::CommandBufferAllocateInfo commandInfo;
        commandInfo.setCommandBufferCount(1);
        commandInfo.setCommandPool(commandPool);
        commandInfo.setLevel(vk::CommandBufferLevel::ePrimary);

        for(int i = 0; i < alignmentInfos.size(); ++i)
        {
            //vertexBuffers[alignmentInfos[i].binding] = VertexBufferInfo();
            vertexBuffers[alignmentInfos[i].binding].size = vertexBufferSizes[i];
        }

        for(auto& vb : vertexBuffers)
        {
/*            vk::BufferCreateInfo vBufferInfo;
            vBufferInfo.setSize(vb.second.size);
            vBufferInfo.setUsage(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
            vBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
            vBufferInfo.setQueueFamilyIndexCount(1);
            vBufferInfo.setPQueueFamilyIndices(&queueFamIndex);
            if(logicalDevice.createBuffer(&vBufferInfo, nullptr, &vb.second.buffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create buffer!\n");

            vk::MemoryRequirements vBufferMemoryReq;
            logicalDevice.getBufferMemoryRequirements(vb.second.buffer, &vBufferMemoryReq);
            system::MemoryAllocationInfo vAllocInfo;
            vAllocInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
            vAllocInfo.size = vBufferMemoryReq.size;
            vAllocInfo.memoryTypeBits = vBufferMemoryReq.memoryTypeBits;
            vAllocInfo.alignment = vBufferMemoryReq.alignment;
            vb.second.memoryData = system::MemoryManager::getInstance()->allocateMemoryLazy(vAllocInfo);*/
            vb.second.buffer.create(vb.second.size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, true, false);

            if(logicalDevice.createFence(&fenceInfo, nullptr, &vb.second.updatedFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");
            if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &vb.second.updatedSemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");

            if(logicalDevice.allocateCommandBuffers(&commandInfo, &vb.second.updateCommandBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate command buffer!\n");
        }

        if(indexBufferSize != 0)
        {
            /*vk::BufferCreateInfo iBufferInfo;
            iBufferInfo.setSize(indexBufferSize);
            iBufferInfo.setUsage(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
            iBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
            iBufferInfo.setQueueFamilyIndexCount(1);
            iBufferInfo.setPQueueFamilyIndices(&queueFamIndex);
            if(logicalDevice.createBuffer(&iBufferInfo, nullptr, &indexBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create buffer!\n");

            vk::MemoryRequirements iBufferMemoryReq;
            logicalDevice.getBufferMemoryRequirements(indexBuffer, &iBufferMemoryReq);
            system::MemoryAllocationInfo iAllocInfo;
            iAllocInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
            iAllocInfo.size = iBufferMemoryReq.size;
            iAllocInfo.alignment = iBufferMemoryReq.alignment;
            iAllocInfo.memoryTypeBits = iBufferMemoryReq.memoryTypeBits;
            indexMemoryData = system::MemoryManager::getInstance()->allocateMemoryLazy(iAllocInfo);*/
            indexBuffer.create(indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, true, false);

            if(logicalDevice.createFence(&fenceInfo, nullptr, &indexBufferUpdatedFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");
            if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &indexBufferUpdatedSemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");

            if(logicalDevice.allocateCommandBuffers(&commandInfo, &indexUpdateCommandBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate command buffer!\n");
        }
    }

    void VertexBuffer::updateVertexBuffer(const void* data, uint32_t binding)
    {
        update(data, true, binding);
    }

    void VertexBuffer::updateIndexBuffer(const void* data)
    {
        if(indexBufferSize == 0) throw std::runtime_error("Trying to update empty index buffer.\n");
        update(data, false, 0);
    }

    void VertexBuffer::update(const void* data, bool vertex, const uint32_t binding)
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::Queue& graphicsQueue = system::Executives::getInstance()->getGraphicsQueue();
        vk::CommandBuffer& updateCommandBuffer = vertex ? vertexBuffers[binding].updateCommandBuffer : indexUpdateCommandBuffer;

        bindMemory();

/*        vk::Buffer transmissionBuffer;
        vk::BufferCreateInfo transmissionBufferInfo;
        transmissionBufferInfo.setQueueFamilyIndexCount(1);
        uint32_t index = system::Executives::getInstance()->getGraphicsQueueFamilyIndex();
        transmissionBufferInfo.setPQueueFamilyIndices(&index);
        transmissionBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        transmissionBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
        transmissionBufferInfo.setSize(vertex ? vertexBuffers[binding].size : indexBufferSize);
        if(logicalDevice.createBuffer(&transmissionBufferInfo, nullptr, &transmissionBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create staging buffer!\n");

        vk::MemoryRequirements transmissionBufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(transmissionBuffer, &transmissionBufferMemoryRequirements);
        system::MemoryAllocationInfo allocInfo;
        allocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;         // TODO: also make memory property non-coherent
        allocInfo.memoryTypeBits = transmissionBufferMemoryRequirements.memoryTypeBits;
        allocInfo.size = transmissionBufferMemoryRequirements.size;
        allocInfo.alignment = transmissionBufferMemoryRequirements.alignment;
        system::AllocatedMemoryData transmissionBufferData = system::MemoryManager::getInstance()->allocateMemory(allocInfo);
        const vk::DeviceMemory& transmissionBufferMemory = system::MemoryManager::getInstance()->getMemory(transmissionBufferData.index);

        if(logicalDevice.bindBufferMemory(transmissionBuffer, transmissionBufferMemory, transmissionBufferData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory!\n");
        void * mappedMemory;
        if(logicalDevice.mapMemory(transmissionBufferMemory, transmissionBufferData.offset, transmissionBufferInfo.size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
        memcpy(mappedMemory, data, vertex ? vertexBuffers[binding].size : indexBufferSize);
        logicalDevice.unmapMemory(transmissionBufferMemory);*/
        utils::Buffer transmissionBuffer;
        transmissionBuffer.create(vertex ? vertexBuffers[binding].size : indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, false, true);
        transmissionBuffer.bindMemory();
        transmissionBuffer.updateCPUAccessible(data);

        if(vertex)
        {
            vertexBuffers[binding].buffer.updateDeviceLocal(updateCommandBuffer, 
                transmissionBuffer.getBuffer(), 
                0, 
                vk::Semaphore(), 
                vertexBuffers[binding].updatedSemaphore, 
                vk::Fence(), 
                vertexBuffers[binding].updatedFence, 
                vk::PipelineStageFlagBits::eVertexInput, 
                true);
        }
        else
        {
            indexBuffer.updateDeviceLocal(updateCommandBuffer,
                transmissionBuffer.getBuffer(),
                0,
                vk::Semaphore(),
                indexBufferUpdatedSemaphore,
                vk::Fence(),
                indexBufferUpdatedFence,
                vk::PipelineStageFlagBits::eVertexInput, 
                true);
        }

/*        vk::CommandBufferBeginInfo commandBufferInfo;
        //commandBufferInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        if(updateCommandBuffer.begin(&commandBufferInfo) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");

        vk::BufferCopy transmissionCopyData;
        transmissionCopyData.setSrcOffset(0);
        transmissionCopyData.setDstOffset(0);
        transmissionCopyData.setSize(vertex ? vertexBuffers[binding].size : indexBufferSize);
        updateCommandBuffer.copyBuffer(transmissionBuffer, vertex ? vertexBuffers[binding].buffer : indexBuffer, 1, &transmissionCopyData);

        updateCommandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&updateCommandBuffer);
        submitInfo.setSignalSemaphoreCount(1);                 
        submitInfo.setPSignalSemaphores(vertex ? &vertexBuffers[binding].updatedSemaphore : &indexBufferUpdatedSemaphore);
        submitInfo.setWaitSemaphoreCount(0);                   
        submitInfo.setPWaitSemaphores(nullptr);                 
        vk::PipelineStageFlags dstStageFlags = vk::PipelineStageFlagBits::eVertexInput;
        submitInfo.setPWaitDstStageMask(&dstStageFlags);

        graphicsQueue.submit(1, &submitInfo, vertex ? vertexBuffers[binding].updatedFence : indexBufferUpdatedFence);*/

        logicalDevice.waitForFences(1, vertex ? &vertexBuffers[binding].updatedFence : &indexBufferUpdatedFence, true, ~0U);              //  move the sync operations out of here
        if(updateCommandBuffer.reset(vk::CommandBufferResetFlags()) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset buffer!\n");
        /*logicalDevice.destroyBuffer(transmissionBuffer, nullptr);
        system::MemoryManager::getInstance()->freeMemory(transmissionBufferData.index);*/
    }

    void VertexBuffer::bindMemory()
    {
        static bool memoryBound = false;
        if(!memoryBound)
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            if(indexBufferSize != 0)
            {
                indexBuffer.bindMemory();
            }

            for(auto& vb : vertexBuffers)
            {
                vb.second.buffer.bindMemory();
            }
            memoryBound = true;
        }
    }

    void VertexBuffer::destroy()
    {
        if(transferred) return;
        if(!vertexBuffers.begin()->second.buffer.getBuffer() && !indexBuffer.getBuffer()) return;
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();

        for(auto& vb : vertexBuffers)
        {
            vb.second.buffer.destroy();
            logicalDevice.destroyFence(vb.second.updatedFence, nullptr);
            vb.second.updatedFence = vk::Fence();
            logicalDevice.destroySemaphore(vb.second.updatedSemaphore, nullptr);
            vb.second.updatedSemaphore = vk::Semaphore();
        }

        if(indexBufferSize != 0)
        {
            indexBuffer.destroy();
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