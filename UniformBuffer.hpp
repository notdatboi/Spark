#ifndef SPARK_UNIFORM_BUFFER_HPP
#define SPARK_UNIFORM_BUFFER_HPP

#include"MemoryManager.hpp"
#include"System.hpp"
#include"Executives.hpp"

namespace spk
{

    class UniformBuffer
    {
    public:
        UniformBuffer();
        UniformBuffer(const UniformBuffer& ub);
        UniformBuffer(UniformBuffer&& ub);
        UniformBuffer(const size_t cSize/*, const bool cDeviceLocal = false*/, uint32_t cSetIndex, uint32_t cBinding);
        void create(const size_t cSize/*, const bool cDeviceLocal = false*/, uint32_t cSetIndex, uint32_t cBinding);
        UniformBuffer& operator=(const UniformBuffer& rBuffer);
        UniformBuffer& operator=(UniformBuffer& rBuffer);
        UniformBuffer& operator=(UniformBuffer&& rBuffer);
        void resetSetIndex(const uint32_t newIndex);
        void resetBinding(const uint32_t newBinding);
       ~UniformBuffer();
    private:
        friend class ResourceSet;
        void bindMemory();
        void update(const void* data);
        const vk::Buffer& getBuffer() const;
        vk::Buffer& getBuffer();
        const vk::Event& getReadyEvent() const;
        const vk::DeviceSize getSize() const;
        const uint32_t getSet() const;
        const uint32_t getBinding() const;

        vk::Buffer buffer;
        size_t size;
        system::AllocatedMemoryData memoryData;
        vk::Event bufferReadyEvent;
        uint32_t setIndex;
        uint32_t binding;
        bool transferred = false;

        void destroy();
    };

}

#endif