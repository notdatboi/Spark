#include"UniformBuffer.hpp"

namespace spk
{
    UniformBuffer::UniformBuffer(){}

    const vk::DeviceSize UniformBuffer::getOffset() const
    {
        return memoryData.offset;
    }

    const vk::Buffer& UniformBuffer::getBuffer() const
    {
        return buffer;
    }

    vk::Buffer& UniformBuffer::getBuffer()
    {
        return buffer;
    }

    const vk::Event& UniformBuffer::getReadyEvent() const
    {
        return bufferReadyEvent;
    }

    UniformBuffer::UniformBuffer(const size_t cSize/*, const bool cDeviceLocal*/, uint32_t cSetIndex, uint32_t cBinding)
    {
        create(cSize/*, cDeviceLocal*/, cSetIndex, cBinding);
    }

    UniformBuffer& UniformBuffer::operator=(UniformBuffer&& rBuffer)
    {
        rBuffer.transferred = true;
        buffer = std::move(rBuffer.buffer);
        size = std::move(rBuffer.size);
        memoryData = std::move(rBuffer.memoryData);
        bufferReadyEvent = std::move(rBuffer.bufferReadyEvent);
        setIndex = std::move(rBuffer.setIndex);
        binding = std::move(rBuffer.binding);
        return *this;
    }

    const uint32_t UniformBuffer::getSet() const
    {
        return setIndex;
    }

    const uint32_t UniformBuffer::getBinding() const
    {
        return binding;
    }

    const vk::DeviceSize UniformBuffer::getSize() const
    {
        return size;
    }

    void UniformBuffer::create(const size_t cSize/*, const bool cDeviceLocal*/, uint32_t cSetIndex, uint32_t cBinding)
    {
        setIndex = cSetIndex;
        binding = cBinding;
    //    rawBufferData = rawData;
        //deviceLocal = cDeviceLocal;
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        vk::BufferCreateInfo createInfo;
        createInfo.setQueueFamilyIndexCount(1);
        uint32_t graphicsFamilyIndex = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        createInfo.setPQueueFamilyIndices(&graphicsFamilyIndex);
        createInfo.setSharingMode(vk::SharingMode::eExclusive);
        createInfo.setSize(cSize);
        size = cSize;
        createInfo.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

        if(logicalDevice.createBuffer(&createInfo, nullptr, &buffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create uniform buffer!\n");
        vk::MemoryRequirements bufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(buffer, &bufferMemoryRequirements);
        MemoryAllocationInfo allocInfo;
        allocInfo.size = bufferMemoryRequirements.size;
        allocInfo.memoryTypeBits = bufferMemoryRequirements.memoryTypeBits;
        /*if(deviceLocal)
        {
            allocInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        }
        else */allocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;        // TODO: change host coherency
        memoryData = MemoryManager::getInstance()->allocateMemoryLazy(allocInfo);

        vk::EventCreateInfo eventInfo;
        if(logicalDevice.createEvent(&eventInfo, nullptr, &bufferReadyEvent) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");
    }

    void UniformBuffer::update(const void* data)
    {
//        if(deviceLocal) throw std::runtime_error("Command buffer is required to write to device local memory.\n");
        static bool memoryBound = false;
        const vk::DeviceMemory& memory = MemoryManager::getInstance()->getMemory(memoryData.index);
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        logicalDevice.resetEvent(bufferReadyEvent);

        if(!memoryBound)
        {
            memoryBound = true;
            if(logicalDevice.bindBufferMemory(buffer, memory, memoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory to buffer!\n");
        }
        
        void* mappedMemory;
        if(logicalDevice.mapMemory(memory, memoryData.offset, size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
        memcpy(mappedMemory, data, size);
        logicalDevice.unmapMemory(memory);
        logicalDevice.setEvent(bufferReadyEvent);
    }

    /*void UniformBuffer::update(const vk::CommandBuffer& memoryBindBuffer, const void* data)
    {
        if(!deviceLocal) throw std::runtime_error("Do not use update with command buffers on non-device-local memory.\n");
        static bool memoryBound = false;
        const vk::DeviceMemory& memory = MemoryManager::getInstance()->getMemory(memoryData.index);
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        const vk::Queue& graphicsQueue = Executives::getInstance()->getGraphicsQueue();

        if(logicalDevice.resetFences(1, &bufferReadyFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset fence!\n");

        if(!memoryBound)
        {
            memoryBound = true;
            if(logicalDevice.bindBufferMemory(buffer, memory, memoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory to buffer!\n");
        }

        vk::BufferCreateInfo dataBufferInfo;
        dataBufferInfo.setQueueFamilyIndexCount(1);
        uint32_t graphicsFamilyIndex = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        dataBufferInfo.setPQueueFamilyIndices(&graphicsFamilyIndex);
        dataBufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        dataBufferInfo.setSize(size);
        dataBufferInfo.setUsage(vk::BufferUsageFlagBits::eTransferSrc);

        vk::Buffer dataBuffer;
        if(logicalDevice.createBuffer(&dataBufferInfo, nullptr, &dataBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create staging buffer!\n");
        vk::MemoryRequirements dataBufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(dataBuffer, &dataBufferMemoryRequirements);
        MemoryAllocationInfo dataBufferMemoryAllocInfo;
        dataBufferMemoryAllocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        dataBufferMemoryAllocInfo.memoryTypeBits = dataBufferMemoryRequirements.memoryTypeBits;
        dataBufferMemoryAllocInfo.size = dataBufferMemoryRequirements.size;
        AllocatedMemoryData dataBufferMemoryData = MemoryManager::getInstance()->allocateMemory(dataBufferMemoryAllocInfo);
        vk::DeviceMemory& dataBufferMemory = MemoryManager::getInstance()->getMemory(dataBufferMemoryData.index);

        void * mappedMemory;
        if(logicalDevice.mapMemory(dataBufferMemory, dataBufferMemoryData.offset, size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
        memcpy(mappedMemory, data, size);
        logicalDevice.unmapMemory(dataBufferMemory);
        if(logicalDevice.bindBufferMemory(dataBuffer, dataBufferMemory, dataBufferMemoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory to staging buffer!\n");

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        if(memoryBindBuffer.begin(&beginInfo) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");

        vk::BufferCopy copyData;
        copyData.setSrcOffset(dataBufferMemoryData.offset);
        copyData.setDstOffset(memoryData.offset);
        copyData.setSize(size);
        memoryBindBuffer.copyBuffer(dataBuffer, buffer, 1, &copyData);

        vk::BufferMemoryBarrier memoryBarrier;
        memoryBarrier.setBuffer(buffer);
        memoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        memoryBarrier.setDstAccessMask(vk::AccessFlagBits::eUniformRead);
        memoryBarrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        memoryBarrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        memoryBarrier.setOffset(memoryData.offset);
        memoryBarrier.setSize(size);

        memoryBindBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexShader,vk::DependencyFlags(), 0, nullptr, 1, &memoryBarrier, 0, nullptr);

        memoryBindBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(&memoryBindBuffer);
        submitInfo.setSignalSemaphoreCount(0);
        submitInfo.setWaitSemaphoreCount(0);
        vk::PipelineStageFlags pipelineFlags = vk::PipelineStageFlagBits::eVertexShader;
        submitInfo.setPWaitDstStageMask(&pipelineFlags);

        if(graphicsQueue.submit(1, &submitInfo, bufferReadyFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to submit queue!\n");

        logicalDevice.waitForFences(1, &bufferReadyFence, true, ~0U);

        logicalDevice.destroyBuffer(dataBuffer, nullptr);
        MemoryManager::getInstance()->freeMemory(dataBufferMemoryData.index);
        if(memoryBindBuffer.reset(vk::CommandBufferResetFlags()) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset command buffer!\n");
    }*/

    UniformBuffer::~UniformBuffer()
    {
        if(!transferred)
        {
            const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
            logicalDevice.destroyBuffer(buffer, nullptr);
            MemoryManager::getInstance()->freeMemory(memoryData.index);
            logicalDevice.destroyEvent(bufferReadyEvent, nullptr);
        }
    }
}