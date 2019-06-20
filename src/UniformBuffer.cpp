#include"../include/UniformBuffer.hpp"

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
    }

    void UniformBuffer::update(const void* data)
    {
        if(data != nullptr)
        {
            buffer.updateCPUAccessible(data);
        }
    }

    void UniformBuffer::bindMemory()
    {
        buffer.bindMemory();
    }

    void UniformBuffer::destroy()
    {
        buffer.destroy();
    }

    UniformBuffer::~UniformBuffer()
    {
        destroy();
    }
}