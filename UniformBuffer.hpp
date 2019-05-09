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
        UniformBuffer(const size_t cSize/*, const bool cDeviceLocal = false*/, uint32_t cSetIndex, uint32_t cBinding);
        void create(const size_t cSize/*, const bool cDeviceLocal = false*/, uint32_t cSetIndex, uint32_t cBinding);
        const vk::Buffer& getBuffer() const;
        vk::Buffer& getBuffer();
        const vk::Fence& getReadyFence() const;
        //void update(const vk::CommandBuffer& memoryBindBuffer, const void* data);
        void update(const void* data);
        const vk::DeviceSize getOffset() const;
        UniformBuffer& operator=(const UniformBuffer& rBuffer) = delete;
        UniformBuffer& operator=(UniformBuffer& rBuffer) = delete;
        UniformBuffer& operator=(UniformBuffer&& rBuffer);
        ~UniformBuffer();
    private:
        vk::Buffer buffer;
        size_t size;
        AllocatedMemoryData memoryData;
        //bool deviceLocal;
        vk::Fence bufferReadyFence;
        uint32_t setIndex;
        uint32_t binding;
        bool transferred = false;
    };

}

#endif