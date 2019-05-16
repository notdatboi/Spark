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
        const vk::Buffer& getBuffer() const;
        vk::Buffer& getBuffer();
        const vk::Event& getReadyEvent() const;
        //void update(const vk::CommandBuffer& memoryBindBuffer, const void* data);
        void update(const void* data);
        const vk::DeviceSize getSize() const;
        UniformBuffer& operator=(const UniformBuffer& rBuffer);
        UniformBuffer& operator=(UniformBuffer& rBuffer);
        UniformBuffer& operator=(UniformBuffer&& rBuffer);
        void resetSetIndex(const uint32_t newIndex);
        void resetBinding(const uint32_t newBinding);
        const uint32_t getSet() const;
        const uint32_t getBinding() const;
       ~UniformBuffer();
    private:
        vk::Buffer buffer;
        size_t size;
        AllocatedMemoryData memoryData;
        //bool deviceLocal;
        vk::Event bufferReadyEvent;
        uint32_t setIndex;
        uint32_t binding;
        bool transferred = false;

        void destroy();
    };

}

#endif