#include"UniformBuffer.hpp"

namespace spk
{
    UniformBuffer::UniformBuffer(){}

    UniformBuffer::UniformBuffer(const UniformBuffer& ub)
    {
        create(ub.size, ub.setIndex, ub.binding);
    }

    UniformBuffer::UniformBuffer(UniformBuffer&& ub)
    {
        ub.transferred = true;
        buffer = std::move(ub.buffer);
        size = std::move(ub.size);
        memoryData = std::move(ub.memoryData);
        bufferReadyEvent = std::move(ub.bufferReadyEvent);
        setIndex = std::move(ub.setIndex);
        binding = std::move(ub.binding);
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

    UniformBuffer& UniformBuffer::operator=(const UniformBuffer& rBuffer)
    {
        destroy();
        create(rBuffer.size, rBuffer.setIndex, rBuffer.binding);
        return *this;
    }

    UniformBuffer& UniformBuffer::operator=(UniformBuffer& rBuffer)
    {
        destroy();
        create(rBuffer.size, rBuffer.setIndex, rBuffer.binding);
        return *this;
    }

    UniformBuffer& UniformBuffer::operator=(UniformBuffer&& rBuffer)
    {
        destroy();
        rBuffer.transferred = true;
        buffer = std::move(rBuffer.buffer);
        size = std::move(rBuffer.size);
        memoryData = std::move(rBuffer.memoryData);
        bufferReadyEvent = std::move(rBuffer.bufferReadyEvent);
        setIndex = std::move(rBuffer.setIndex);
        binding = std::move(rBuffer.binding);
        return *this;
    }

    void UniformBuffer::resetSetIndex(const uint32_t newIndex)
    {
        setIndex = newIndex;
    }

    void UniformBuffer::resetBinding(const uint32_t newBinding)
    {
        binding = newBinding;
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
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        vk::BufferCreateInfo createInfo;
        createInfo.setQueueFamilyIndexCount(1);
        uint32_t graphicsFamilyIndex = system::Executives::getInstance()->getGraphicsQueueFamilyIndex();
        createInfo.setPQueueFamilyIndices(&graphicsFamilyIndex);
        createInfo.setSharingMode(vk::SharingMode::eExclusive);
        createInfo.setSize(cSize);
        size = cSize;
        createInfo.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

        if(logicalDevice.createBuffer(&createInfo, nullptr, &buffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create uniform buffer!\n");
        vk::MemoryRequirements bufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(buffer, &bufferMemoryRequirements);
        system::MemoryAllocationInfo allocInfo;
        allocInfo.size = bufferMemoryRequirements.size;
        allocInfo.memoryTypeBits = bufferMemoryRequirements.memoryTypeBits;
        allocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;        // TODO: change host coherency
        allocInfo.alignment = bufferMemoryRequirements.alignment;
        memoryData = system::MemoryManager::getInstance()->allocateMemoryLazy(allocInfo);

        vk::EventCreateInfo eventInfo;
        if(logicalDevice.createEvent(&eventInfo, nullptr, &bufferReadyEvent) != vk::Result::eSuccess) throw std::runtime_error("Failed to create fence!\n");
        if(logicalDevice.resetEvent(bufferReadyEvent) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset event!\n");
    }

    void UniformBuffer::update(const void* data)
    {
        if(data != nullptr)
        {
            const vk::DeviceMemory& memory = system::MemoryManager::getInstance()->getMemory(memoryData.index);
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            if(logicalDevice.resetEvent(bufferReadyEvent) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset event!\n");
            
            void* mappedMemory;
            if(logicalDevice.mapMemory(memory, memoryData.offset, size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
            memcpy(mappedMemory, data, size);
            logicalDevice.unmapMemory(memory);
            logicalDevice.setEvent(bufferReadyEvent);
        }
    }

    void UniformBuffer::bindMemory()
    {
        //static bool memoryBound = false;      // TODO: add validation for memory binding
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::DeviceMemory& memory = system::MemoryManager::getInstance()->getMemory(memoryData.index);
        if(logicalDevice.resetEvent(bufferReadyEvent) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset event!\n");

        if(logicalDevice.bindBufferMemory(buffer, memory, memoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory to buffer!\n");

        logicalDevice.setEvent(bufferReadyEvent);
    }

    void UniformBuffer::destroy()
    {
        if(!transferred)                                                                    // If buffer wasn't transferred..
        {
            if(buffer.operator VkBuffer() != VK_NULL_HANDLE)                                // ..and was properly created and wasn't destroyed, destroy it
            {
                const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
                logicalDevice.destroyBuffer(buffer, nullptr);
                buffer = VkBuffer(0);
                system::MemoryManager::getInstance()->freeMemory(memoryData.index);
                logicalDevice.destroyEvent(bufferReadyEvent, nullptr);
                bufferReadyEvent = VkEvent(0);
            }
        }
    }

    UniformBuffer::~UniformBuffer()
    {
        destroy();
    }
}