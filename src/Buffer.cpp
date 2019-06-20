#include"../include/Buffer.hpp"

namespace spk
{
    namespace utils
    {
        Buffer::Buffer()
        {
            memoryData.index = ~0;
            memoryData.offset = ~0;
        }

        Buffer::Buffer(const vk::DeviceSize cSize, const vk::BufferUsageFlags cUsage, const bool cDeviceLocal, const bool cInstantAllocation)
        {
            memoryData.index = ~0;
            memoryData.offset = ~0;
            create(cSize, cUsage, cDeviceLocal, cInstantAllocation);
        }

        Buffer::Buffer(const Buffer& buf)
        {
            destroy();
            if(buf.buffer)
            {
                create(buf.size, buf.usage, buf.deviceLocal, buf.instantAlloc);
            }
        }

        Buffer& Buffer::operator=(const Buffer& buf)
        {
            destroy();
            if(buf.buffer)
            {
                create(buf.size, buf.usage, buf.deviceLocal, buf.instantAlloc);
            }
            return *this;
        }

        void Buffer::create(const vk::DeviceSize cSize, const vk::BufferUsageFlags cUsage, const bool cDeviceLocal, const bool cInstantAllocation)
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            size = cSize;
            usage = cUsage;
            deviceLocal = cDeviceLocal;
            instantAlloc = cInstantAllocation;

            const auto graphicsFamIndex = system::Executives::getInstance()->getGraphicsQueueFamilyIndex();

            vk::BufferCreateInfo info;
            info.setSize(size);
            info.setUsage(usage);
            info.setSharingMode(vk::SharingMode::eExclusive);
            info.setQueueFamilyIndexCount(1);
            info.setPQueueFamilyIndices(&graphicsFamIndex);

            if(logicalDevice.createBuffer(&info, nullptr, &buffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to create buffer!\n");

            vk::MemoryRequirements memoryRequirements;
            logicalDevice.getBufferMemoryRequirements(buffer, &memoryRequirements);

            system::MemoryAllocationInfo memoryInfo;
            memoryInfo.size = memoryRequirements.size;
            memoryInfo.memoryTypeBits = memoryRequirements.memoryTypeBits;
            memoryInfo.alignment = memoryRequirements.alignment;
            memoryInfo.flags = (deviceLocal ? vk::MemoryPropertyFlagBits::eDeviceLocal : vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

            if(!instantAlloc)
            {
                memoryData = system::MemoryManager::getInstance()->allocateMemoryLazy(memoryInfo);
            }
            else
            {
                memoryData = system::MemoryManager::getInstance()->allocateMemory(memoryInfo);
            }
        }

        void Buffer::bindMemory()
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            const vk::DeviceMemory& memory = system::MemoryManager::getInstance()->getMemory(memoryData.index);
            logicalDevice.bindBufferMemory(buffer, memory, memoryData.offset);
        }

        void Buffer::updateCPUAccessible(const void* data)
        {
            if(deviceLocal) throw std::runtime_error("Trying to update device local buffer as CPU-accessible.\n");
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            const vk::DeviceMemory& memory = system::MemoryManager::getInstance()->getMemory(memoryData.index);
            void* mappedMemory;
            if(logicalDevice.mapMemory(memory, memoryData.offset, size, vk::MemoryMapFlags(), &mappedMemory) != vk::Result::eSuccess) throw std::runtime_error("Failed to map memory!\n");
            memcpy(mappedMemory, data, size);
            logicalDevice.unmapMemory(memory);
        }

        void Buffer::updateDeviceLocal(vk::CommandBuffer& updateBuffer,
            const vk::Buffer& copyBuffer,
            const vk::DeviceSize srcOffset,
            const vk::Semaphore& waitSemaphore,
            const vk::Semaphore& signalSemaphore,
            const vk::Fence& waitFence,
            const vk::Fence& signalFence,
            const vk::PipelineStageFlags dstStageFlags,
            bool oneTimeSubmit)
        {
            if(!deviceLocal) throw std::runtime_error("Trying to update CPU-accessible buffer as device local.\n");
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            const vk::Queue& graphicsQueue = system::Executives::getInstance()->getGraphicsQueue();

            vk::BufferCopy copyInfo;
            copyInfo.setDstOffset(0);
            copyInfo.setSrcOffset(srcOffset);
            copyInfo.setSize(size);

            if(waitFence)
            {
                if(logicalDevice.waitForFences(1, &waitFence, true, ~0U) != vk::Result::eSuccess) throw std::runtime_error("Failed to wait for fence!\n");
                if(logicalDevice.resetFences(1, &waitFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to wait for fence!\n");
            }

            vk::CommandBufferBeginInfo beginInfo;
            if(oneTimeSubmit) beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

            if(updateBuffer.begin(&beginInfo) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");
            updateBuffer.copyBuffer(copyBuffer, buffer, 1, &copyInfo);
            updateBuffer.end();

            vk::SubmitInfo submit;
            if(waitSemaphore)
            {
                submit.setWaitSemaphoreCount(1);
                submit.setPWaitSemaphores(&waitSemaphore);
            }
            else
            {
                submit.setWaitSemaphoreCount(0);
                submit.setPWaitSemaphores(nullptr);
            }
            submit.setPWaitDstStageMask(&dstStageFlags);
            submit.setCommandBufferCount(1);
            submit.setPCommandBuffers(&updateBuffer);
            if(signalSemaphore)
            {
                submit.setSignalSemaphoreCount(1);
                submit.setPSignalSemaphores(&signalSemaphore);
            }
            else
            {
                submit.setSignalSemaphoreCount(0);
                submit.setPSignalSemaphores(nullptr);
            }

            if(signalFence)
            {
                if(graphicsQueue.submit(1, &submit, signalFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to submit queue!\n");
            }
            else
            {
                if(graphicsQueue.submit(1, &submit, vk::Fence()) != vk::Result::eSuccess) throw std::runtime_error("Failed to submit queue!\n");
            }
        }

        void Buffer::destroy()
        {
            if(buffer)
            {
                const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
                logicalDevice.destroyBuffer(buffer, nullptr);
                buffer = vk::Buffer();
            }
            if(memoryData.index != (~0) && memoryData.offset != (~0))
            {
                system::MemoryManager::getInstance()->freeMemory(memoryData.index);
                memoryData.index = ~0;
                memoryData.offset = ~0;
            }
        }

        const vk::Buffer& Buffer::getBuffer() const
        {
            return buffer;
        }

        Buffer::~Buffer()
        {
            destroy();
        }
    }
}