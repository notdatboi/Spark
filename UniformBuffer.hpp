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
        UniformBuffer(const size_t cSize/*, const bool cDeviceLocal = false*/);
        void create(const size_t cSize/*, const bool cDeviceLocal = false*/);
        const vk::Buffer& getBuffer() const;
        vk::Buffer& getBuffer();
        //void update(const vk::CommandBuffer& memoryBindBuffer, const void* data);
        void update(const void* data);
        const vk::DeviceSize getOffset() const;
        ~UniformBuffer();
    private:
        vk::Buffer buffer;
        size_t size;
        AllocatedMemoryData memoryData;
        //bool deviceLocal;
        vk::Fence bufferReadyFence;
    };

}

#endif