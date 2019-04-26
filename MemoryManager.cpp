#include"MemoryManager.hpp"
#include"System.hpp"

namespace spk
{

    MemoryManager* MemoryManager::instance = nullptr;

    MemoryManager::MemoryManager()
    {
        const vk::PhysicalDevice& physicalDevice = System::getInstance()->getPhysicalDevice();
        physicalDevice.getMemoryProperties(&memoryProperties);
    }

    MemoryManager* MemoryManager::getInstance()
    {
        static bool created = false;
        if(!created)
        {
            created = true;
            instance = new MemoryManager();
        }
        return instance;
    }

    uint32_t MemoryManager::findMemoryTypeIndex(vk::MemoryPropertyFlags flags) const
    {
        for(int i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            if((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) return i;
        }
        throw std::runtime_error("Failed to find requested memory propery flags!\n");
    }

    index_t MemoryManager::allocateMemoryBlock(const MemoryAllocationInfo& info)
    {
        index_t index;
        if(freedIndices.size() == 0)
        {
            memoryArray.push_back(vk::DeviceMemory());
            index = memoryArray.size() - 1;
            memoryPartitionsCount.push_back(1);
        }
        else
        {
            index = *freedIndices.begin();
            memoryPartitionsCount[index]++;
            freedIndices.erase(freedIndices.begin());
        }
        allocateMemoryBlock(info, index);
        return index;
    }

    void MemoryManager::allocateMemoryBlock(const MemoryAllocationInfo& info, index_t index)
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        vk::MemoryAllocateInfo vkInfo;
        vkInfo.setAllocationSize(info.size);
        vkInfo.setMemoryTypeIndex(findMemoryTypeIndex(info.flags));
        if(logicalDevice.allocateMemory(&vkInfo, nullptr, &memoryArray[index]) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to allocate memory!\n");
        }
    }

    void MemoryManager::flushLazyAllocations()
    {
        for(const auto& allocation : pendingAllocations)
        {
            flushLazyAllocationsByFlags(allocation.second.flags);
        }
    }

    void MemoryManager::flushLazyAllocationsByFlags(const vk::MemoryPropertyFlags flags)
    {
        std::string sFlags = vk::to_string(flags);
    #ifndef DEBUG
        if(pendingAllocations.count(sFlags) == 0) return;
    #else
        if(pendingAllocations.count(sFlags) == 0) throw std::runtime_error("No pending allocations of this memory type!\n");
    #endif
        auto data = pendingAllocations[sFlags];
        MemoryAllocationInfo info;
        info.flags = flags;
        info.size = data.size;
        allocateMemoryBlock(info, data.index);
        auto indexIterator = lazilyAllocatedIndices.find(data.index);
        lazilyAllocatedIndices.erase(indexIterator);
        pendingAllocations.erase(sFlags);
    }

    AllocatedMemoryData MemoryManager::allocateMemory(const MemoryAllocationInfo& allocationInfo)
    {
        index_t allocationIndex = allocateMemoryBlock(allocationInfo);
        return {allocationIndex, vk::DeviceSize(0)};
    }

    AllocatedMemoryData MemoryManager::allocateMemoryLazy(const MemoryAllocationInfo& allocationInfo)
    {
        index_t index;
        vk::DeviceSize offset = 0;
        std::string sFlags = vk::to_string(allocationInfo.flags);
        if(freedIndices.size() == 0)
        {
            memoryArray.push_back(vk::DeviceMemory());
            index = memoryArray.size() - 1;
        }
        else
        {
            index = *freedIndices.begin();
            freedIndices.erase(freedIndices.begin());
        }
        if(pendingAllocations.count(sFlags) == 0)
        {
            pendingAllocations[sFlags] = {index, allocationInfo.size, allocationInfo.flags};
        }
        else
        {
            offset = pendingAllocations[sFlags].size;
            pendingAllocations[sFlags].size += allocationInfo.size;
            index = pendingAllocations[sFlags].index;
        }
        lazilyAllocatedIndices.insert(index);
        return {index, offset};
    }

    vk::DeviceMemory& MemoryManager::getMemory(const index_t index)
    {
        if(lazilyAllocatedIndices.find(index) != lazilyAllocatedIndices.end())
        {
            for(const auto& data : pendingAllocations)
            {
                if(data.second.index == index)
                {
                    flushLazyAllocationsByFlags(data.second.flags);
                    break;
                }
            }
        }
        if(memoryArray[index].operator VkDeviceMemory() == VK_NULL_HANDLE) throw std::runtime_error("Failed to get memory!\n");
        return memoryArray[index];
    }

    void MemoryManager::freeMemory(const index_t index)
    {
        if(memoryPartitionsCount[index] <= 0) throw std::runtime_error("Trying to free freed memory!\n");
        if(memoryPartitionsCount[index] == 1)
        {
            const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
            logicalDevice.freeMemory(memoryArray[index], nullptr);
            freedIndices.insert(index);
        }
        else memoryPartitionsCount[index]--;
    }

}