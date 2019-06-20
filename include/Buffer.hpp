#ifndef SPARK_BUFFER_HPP
#define SPARK_BUFFER_HPP

#include"MemoryManager.hpp"
#include"System.hpp"
#include"Executives.hpp"

namespace spk
{
    namespace utils
    {
        class Buffer
        {
        public:
            Buffer();
            Buffer(const vk::DeviceSize cSize, const vk::BufferUsageFlags cUsage, const bool cDeviceLocal, const bool cInstantAllocation);
            Buffer(const Buffer& buf);
            Buffer& operator=(const Buffer& buf);
            void create(const vk::DeviceSize cSize, const vk::BufferUsageFlags cUsage, const bool cDeviceLocal, const bool cInstantAllocation);
            void bindMemory();
            void updateDeviceLocal(vk::CommandBuffer& updateBuffer,
                const vk::Buffer& copyBuffer,
                const vk::DeviceSize srcOffset,
                const vk::Semaphore& waitSemaphore,
                const vk::Semaphore& signalSemaphore,
                const vk::Fence& waitFence,
                const vk::Fence& signalFence,
                const vk::PipelineStageFlags dstStageFlags,
                bool oneTimeSubmit = false);                                          // the buffer data must be tightly packed inside the buffer; offset must be 0
            void updateCPUAccessible(const void* data);
            void destroy();
            const vk::Buffer& getBuffer() const;
            ~Buffer();
        private:
            vk::DeviceSize size;
            vk::BufferUsageFlags usage;
            system::AllocatedMemoryData memoryData;
            vk::Buffer buffer;
            bool instantAlloc;
            bool deviceLocal;
        };
    }
}

#endif