#include"UniformBuffer.hpp"

namespace spk
{
    UniformBuffer::UniformBuffer(){}

    UniformBuffer::UniformBuffer(const UniformBuffer& ub)
    {
        create(ub.size, ub.setIndex, ub.binding);
    }

    const vk::Buffer& UniformBuffer::getBuffer() const
    {
        return buffer.getBuffer();
    }

    UniformBuffer::UniformBuffer(const size_t cSize, const uint32_t cSetIndex, const uint32_t cBinding)
    {
        create(cSize, cSetIndex, cBinding);
    }

    UniformBuffer& UniformBuffer::operator=(const UniformBuffer& rBuffer)
    {
        destroy();
        create(rBuffer.size, rBuffer.setIndex, rBuffer.binding);
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

    void UniformBuffer::create(const size_t cSize, const uint32_t cSetIndex, const uint32_t cBinding)
    {
        setIndex = cSetIndex;
        binding = cBinding;
        size = cSize;
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();

        buffer.create(size, vk::BufferUsageFlagBits::eUniformBuffer, false, false);

/*        vk::BufferCreateInfo createInfo;
        createInfo.setQueueFamilyIndexCount(1);
        uint32_t graphicsFamilyIndex = system::Executives::getInstance()->getGraphicsQueueFamilyIndex();
        createInfo.setPQueueFamilyIndices(&graphicsFamilyIndex);
        createInfo.setSharingMode(vk::SharingMode::eExclusive);
        createInfo.setSize(cSize);
        createInfo.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

        if(logicalDevice.createBuffer(&createInfo, nullptr, &buffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create uniform buffer!\n");
        vk::MemoryRequirements bufferMemoryRequirements;
        logicalDevice.getBufferMemoryRequirements(buffer, &bufferMemoryRequirements);
        system::MemoryAllocationInfo allocInfo;
        allocInfo.size = bufferMemoryRequirements.size;
        allocInfo.memoryTypeBits = bufferMemoryRequirements.memoryTypeBits;
        allocInfo.flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;        // TODO: change host coherency
        allocInfo.alignment = bufferMemoryRequirements.alignment;
        memoryData = system::MemoryManager::getInstance()->allocateMemoryLazy(allocInfo);*/
    }

    void UniformBuffer::update(const void* data)
    {
        if(data != nullptr)
        {
/*            const vk::DeviceMemory& memory = system::MemoryManager::getInstance()->getMemory(memoryData.index);
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            
            void* mappedMemory;
            if(logicalDevice.mapMemory(memory, memoryData.offset, size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
            memcpy(mappedMemory, data, size);
            logicalDevice.unmapMemory(memory);*/
            buffer.updateCPUAccessible(data);
        }
    }

    void UniformBuffer::bindMemory()
    {
        buffer.bindMemory();
/*        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::DeviceMemory& memory = system::MemoryManager::getInstance()->getMemory(memoryData.index);

        if(logicalDevice.bindBufferMemory(buffer, memory, memoryData.offset) != vk::Result::eSuccess) throw std::runtime_error("Failed to bind memory to buffer!\n");*/
    }

    void UniformBuffer::destroy()
    {
        buffer.destroy();
        /*if(buffer.getBuffer())                                // ..and was properly created and wasn't destroyed, destroy it
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            logicalDevice.destroyBuffer(buffer, nullptr);
            buffer = VkBuffer(0);
            system::MemoryManager::getInstance()->freeMemory(memoryData.index);
            logicalDevice.destroyEvent(bufferReadyEvent, nullptr);
            bufferReadyEvent = VkEvent(0);
        }*/
    }

    UniformBuffer::~UniformBuffer()
    {
        destroy();
    }
}