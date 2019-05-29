#ifndef SPARK_MEMORY_MANAGER_HPP
#define SPARK_MEMORY_MANAGER_HPP

#include"SparkIncludeBase.hpp"
#include<set>
#include<map>
#include<memory>
#include<vector>

namespace spk
{
    namespace system
    {
        typedef size_t index_t;

        struct MemoryAllocationInfo
        {
            vk::DeviceSize size;                                                                            // must be integer multiple of memory alignment
            vk::MemoryPropertyFlags flags;
            uint32_t memoryTypeBits;
            vk::DeviceSize alignment;
        };

        struct AllocatedMemoryData
        {
            index_t index;
            vk::DeviceSize offset;
        };

        class MemoryManager
        {
        public:
            static MemoryManager* getInstance();
            AllocatedMemoryData allocateMemory(const MemoryAllocationInfo& allocationInfo);                 // allocates memory at once
            AllocatedMemoryData allocateMemoryLazy(const MemoryAllocationInfo& allocationInfo);             // schedules memory allocation until the first usage
            vk::DeviceMemory& getMemory(const index_t index);                                               // gets memory from specific place in memory array. If this index refers to lazy allocation - allocates it, then returns it
            void freeMemory(const index_t index);                                                           // if memory was lazily allocated, nothing will be freed until all of the parts are asked to be freed
            void flushLazyAllocations();                                                                    // allocates all pending lazy allocations
            void flushLazyAllocationsByFlags(const vk::MemoryPropertyFlags flags);                          // allocates pending lazy allocations with specific flags
            void destroy();
        private:
            struct PendingAllocationData
            {
                index_t index;
                vk::DeviceSize size;
                vk::MemoryPropertyFlags flags;
                uint32_t memoryTypeBits;
                vk::DeviceSize alignment;
            };

            MemoryManager();
            uint32_t findMemoryTypeIndex(const vk::MemoryPropertyFlags flags, uint32_t memoryTypeBits) const;
            index_t allocateMemoryBlock(const MemoryAllocationInfo& info);                                  // allocates memory from free positions in the memory array or creates new device memory object
            void allocateMemoryBlock(const MemoryAllocationInfo& info, index_t index);                      // allocates memory from memory array's specific position

            static std::unique_ptr<MemoryManager> instance;
            vk::PhysicalDeviceMemoryProperties memoryProperties;
            std::map<std::string, PendingAllocationData> pendingAllocations;                                // std::string is stringified value of flags; index_t here refers to index in memory array
            std::vector<vk::DeviceMemory> memoryArray;
            std::set<index_t> freedIndices;
            std::set<index_t> lazilyAllocatedIndices;
            std::vector<size_t> memoryPartitionsCount;
        };
    }
}

#endif