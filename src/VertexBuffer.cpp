#include"../include/VertexBuffer.hpp"

namespace spk
{
    VertexAlignmentInfo::VertexAlignmentInfo(const std::vector<BindingAlignmentInfo>& cBindingAlignmentInfos)
    {
        create(cBindingAlignmentInfos);
    }

    void VertexAlignmentInfo::create(const std::vector<BindingAlignmentInfo>& cBindingAlignmentInfos)
    {
        identifier = count;
        ++count;
        bindingAlignmentInfos = cBindingAlignmentInfos;
    }

    const std::vector<BindingAlignmentInfo>& VertexAlignmentInfo::getAlignmentInfos() const
    {
        return bindingAlignmentInfos;
    }

    const uint32_t VertexAlignmentInfo::getIdentifier() const
    {
        return identifier;
    }

    VertexAlignmentInfo& VertexAlignmentInfo::operator=(const VertexAlignmentInfo& rInfo)
    {
        bindingAlignmentInfos = rInfo.bindingAlignmentInfos;
        identifier = rInfo.identifier;
        return *this;
    }

    uint32_t VertexAlignmentInfo::count = 0;



    VertexBuffer::VertexBuffer(){}

    VertexBuffer::VertexBuffer(const std::vector<uint32_t>& cVertexBufferBindings, const std::vector<uint32_t>& cVertexBufferSizes, const uint32_t cIndexBufferSize): 
        //alignmentInfos(cAlignmentInfos), 
        vertexBufferBindings(cVertexBufferBindings),
        vertexBufferSizes(cVertexBufferSizes),
        indexBufferSize(cIndexBufferSize)
    {
        init();
    }

    VertexBuffer::VertexBuffer(const VertexBuffer& vb)
    {
        create(vb.vertexBufferBindings, vb.vertexBufferSizes, vb.indexBufferSize);
    }

    void VertexBuffer::create(const std::vector<uint32_t>& cVertexBufferBindings, const std::vector<uint32_t>& cVertexBufferSizes, const uint32_t cIndexBufferSize)
    {
        vertexBufferBindings = cVertexBufferBindings;
        vertexBufferSizes = cVertexBufferSizes;
        indexBufferSize = cIndexBufferSize;
        init();
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
        create(rBuffer.vertexBufferBindings, rBuffer.vertexBufferSizes, rBuffer.indexBufferSize);
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

        for(int i = 0; i < vertexBufferBindings.size(); ++i)
        {
            vertexBuffers[vertexBufferBindings[i]].size = vertexBufferSizes[i];
        }

        for(auto& vb : vertexBuffers)
        {
            vb.second.buffer.create(vb.second.size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, true, false);

            if(logicalDevice.createFence(&fenceInfo, nullptr, &vb.second.updatedFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");
            if(logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &vb.second.updatedSemaphore) != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphore!\n");

            if(logicalDevice.allocateCommandBuffers(&commandInfo, &vb.second.updateCommandBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate command buffer!\n");
        }

        if(indexBufferSize != 0)
        {
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

        logicalDevice.waitForFences(1, vertex ? &vertexBuffers[binding].updatedFence : &indexBufferUpdatedFence, true, ~0U);              //  move the sync operations out of here
        updateCommandBuffer.reset(vk::CommandBufferResetFlags());
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