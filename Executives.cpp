#include"Executives.hpp"
#include"System.hpp"
#include"WindowSystem.hpp"

namespace spk
{

    Executives* Executives::executivesInstance = nullptr;

    Executives::Executives()
    {
        uint32_t queueFamilyPropertyCount;
        vk::PhysicalDevice& physicalDevice = System::getInstance()->getPhysicalDevice();
        physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, nullptr);
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, queueFamilyProperties.data());

        uint32_t i = 0;
        vk::Bool32 presentSupport = false;
        bool graphicsSupport = false;
        for(auto& properties : queueFamilyProperties)
        {
            if(!presentSupport)
            {
                if(physicalDevice.getSurfaceSupportKHR(i, *WindowSystem::getInstance(), &presentSupport) != vk::Result::eSuccess)
                {
                    throw std::runtime_error("Failed to get surface support!\n");
                }
                if(presentSupport) presentQueueFamilyIndex = i;
            }
            if(!graphicsSupport && (properties.queueFlags & vk::QueueFlagBits::eGraphics))
            {
                graphicsQueueFamilyIndex = i;
                graphicsSupport = true;
            }
            ++i;
        }
    //   std::cout << '!' << presentSupport << '!' << graphicsSupport << '!';
        if(!(graphicsSupport && presentSupport)) throw std::runtime_error("Failed to pick graphics or present queue!\n");
    }

    Executives* Executives::getInstance()
    {
        static bool created = false;
        static bool queuesObtained = false; // is called from system::getInstance once and then from system::createLogicalDevice twice before needed
        if(!created)
        {
            std::cout << "Creating executives\n";
            executivesInstance = new Executives();
            created = true;
            return executivesInstance;
        }
        if(!queuesObtained)
        {
            vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
            if(logicalDevice.operator VkDevice() == VK_NULL_HANDLE)
            {
                return executivesInstance;
            }
            queuesObtained = true;
            logicalDevice.getQueue(executivesInstance->graphicsQueueFamilyIndex, 0, &executivesInstance->graphicsQueue);
            logicalDevice.getQueue(executivesInstance->presentQueueFamilyIndex, 0, &executivesInstance->presentQueue);
        }
        return executivesInstance;
    }

    const uint32_t Executives::getGraphicsQueueFamilyIndex() const
    {
        return graphicsQueueFamilyIndex;
    }

    const uint32_t Executives::getPresentQueueFamilyIndex() const
    {
        return presentQueueFamilyIndex;
    }

    const vk::Queue& Executives::getGraphicsQueue() const
    {
        return graphicsQueue;
    }

    const vk::Queue& Executives::getPresentQueue() const
    {
        return presentQueue;
    }

    vk::Queue& Executives::getGraphicsQueue()
    {
        return graphicsQueue;
    }

    vk::Queue& Executives::getPresentQueue()
    {
        return presentQueue;
    }

}