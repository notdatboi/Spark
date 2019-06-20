#include"../include/Executives.hpp"
#include"../include/System.hpp"

namespace spk
{
    namespace system
    {
        std::unique_ptr<Executives> Executives::executivesInstance = nullptr;

        Executives::Executives()
        {
            uint32_t queueFamilyPropertyCount;
            const vk::PhysicalDevice& physicalDevice = System::getInstance()->getPhysicalDevice();
            physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, nullptr);
            queueFamilyProperties.resize(queueFamilyPropertyCount);
            physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, queueFamilyProperties.data());

            uint32_t i = 0;
            bool graphicsSupport = false;
            for(auto& properties : queueFamilyProperties)
            {
                if(!graphicsSupport && (properties.queueFlags & vk::QueueFlagBits::eGraphics))
                {
                    graphicsQueueFamilyIndex = i;
                    graphicsSupport = true;
                    break;
                }
                ++i;
            }
            if(!graphicsSupport) throw std::runtime_error("Failed to pick graphics queue!\n");
        }
        
        std::pair<uint32_t, const vk::Queue*> Executives::getPresentQueue(const vk::SurfaceKHR& surface)
        {
            const vk::PhysicalDevice& physicalDevice = System::getInstance()->getPhysicalDevice();
            const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
            vk::Bool32 presentSupport = false;
            uint32_t i = 0;
            for(auto& properties : queueFamilyProperties)
            {
                if(!presentSupport)
                {
                    if(physicalDevice.getSurfaceSupportKHR(i, surface, &presentSupport) != vk::Result::eSuccess)
                    {
                        throw std::runtime_error("Failed to get surface support!\n");
                    }
                    if(presentSupport)
                    {
                        if(presentQueues.count(i) == 0)
                        {
                            logicalDevice.getQueue(i, 0, &presentQueues[i]);
                        }
                        return {i, &presentQueues[i]};
                    }
                }
                ++i;
            }
            throw std::runtime_error("No queues support this surface!\n");
        }

        Executives* Executives::getInstance()
        {
            static bool created = false;
            static bool graphicsQueueObtained = false;
            if(!created)
            {
                executivesInstance.reset(new Executives());
                created = true;
                return executivesInstance.get();
            }
            if(!graphicsQueueObtained)
            {
                const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
                if(logicalDevice.operator VkDevice() == VK_NULL_HANDLE)
                {
                    return executivesInstance.get();
                }
                graphicsQueueObtained = true;
                logicalDevice.getQueue(executivesInstance->graphicsQueueFamilyIndex, 0, &executivesInstance->graphicsQueue);
                executivesInstance->createPool();
            }
            return executivesInstance.get();
        }

        const uint32_t Executives::getGraphicsQueueFamilyIndex() const
        {
            return graphicsQueueFamilyIndex;
        }

        const vk::Queue& Executives::getGraphicsQueue() const
        {
            return graphicsQueue;
        }

        const vk::CommandPool& Executives::getPool() const
        {
            return pool;
        }

        void Executives::createPool()
        {
            const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
            vk::CommandPoolCreateInfo poolInfo;
            poolInfo.setQueueFamilyIndex(graphicsQueueFamilyIndex);
            poolInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
            logicalDevice.createCommandPool(&poolInfo, nullptr, &pool);
        }

        void Executives::destroy()
        {
            if(pool.operator VkCommandPool() != VK_NULL_HANDLE)
            {
                const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
                logicalDevice.destroyCommandPool(pool, nullptr);
            }
        }
    }
}