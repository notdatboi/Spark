#include"../include/MemoryManager.hpp"
#include"../include/System.hpp"

namespace spk
{
    namespace system
    {
        std::unique_ptr<MemoryManager> MemoryManager::instance = nullptr;

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
                instance.reset(new MemoryManager());
            }
            return instance.get();
        }

        uint32_t MemoryManager::findMemoryTypeIndex(vk::MemoryPropertyFlags flags, uint32_t memoryTypeBits) const
        {
            for(int i = 0; i < memoryProperties.memoryTypeCount; ++i)
            {
                if((((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) && ((1 << i) & memoryTypeBits)))
                {
                    return i;
                }
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
            vkInfo.setMemoryTypeIndex(findMemoryTypeIndex(info.flags, info.memoryTypeBits));
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
            info.memoryTypeBits = data.memoryTypeBits;
            info.alignment = data.alignment;
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
            if(pendingAllocations.count(sFlags) == 0)
            {
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
                pendingAllocations[sFlags] = {index, allocationInfo.size, allocationInfo.flags, allocationInfo.memoryTypeBits, allocationInfo.alignment};
            }
            else
            {
                if((pendingAllocations[sFlags].memoryTypeBits & allocationInfo.memoryTypeBits) && pendingAllocations[sFlags].alignment == allocationInfo.alignment)
                {
                    offset = pendingAllocations[sFlags].size;
                    pendingAllocations[sFlags].size += allocationInfo.size;
                    pendingAllocations[sFlags].memoryTypeBits &= allocationInfo.memoryTypeBits;
                    index = pendingAllocations[sFlags].index;
                    memoryPartitionsCount[index]++;
                }
                else
                {
                    flushLazyAllocationsByFlags(allocationInfo.flags);
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
                    pendingAllocations[sFlags] = {index, allocationInfo.size, allocationInfo.flags, allocationInfo.memoryTypeBits, allocationInfo.alignment};
                }
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
            for(auto& pendingAlloc : pendingAllocations)
            {
                if(pendingAlloc.second.index == index)
                {
                    flushLazyAllocationsByFlags(pendingAlloc.second.flags);//throw std::runtime_error("Trying to free memory, that isn't allocated yet!\n");
                    break;
                }
            }
            if(memoryPartitionsCount[index] <= 0)
            {
                return;
            }
            if(memoryPartitionsCount[index] == 1)
            {
                const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
                logicalDevice.freeMemory(memoryArray[index], nullptr);
                memoryArray[index] = vk::DeviceMemory(VkDeviceMemory(VK_NULL_HANDLE));
                freedIndices.insert(index);
                memoryPartitionsCount[index]--;
            }
            else
            {
                memoryPartitionsCount[index]--;
            }
        }

        void MemoryManager::destroy()
        {
            index_t i = 0;
            for(auto& memory : memoryArray)
            {
                while(freedIndices.find(i) == freedIndices.end())
                {
                    freeMemory(i);
                }
                ++i;
            }
        }
    }
}