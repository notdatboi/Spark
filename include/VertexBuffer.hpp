#ifndef SPARK_VERTEX_BUFFER_HPP
#define SPARK_VERTEX_BUFFER_HPP

#include"System.hpp"
#include"MemoryManager.hpp"
#include"Executives.hpp"
#include"Buffer.hpp"
#include<vector>

namespace spk
{
    
    enum class FieldFormat
    {
        int32,
        uint32,
        float32,
        double64,

        vec2i,
        vec2u,
        vec2f,
        vec2d,

        vec3i,
        vec3u,
        vec3f,
        vec3d,

        vec4i,
        vec4u,
        vec4f,
        vec4d
    };

    struct StructFieldInfo
    {
        uint32_t location;
        FieldFormat format;
        uint32_t offset;
    };

    struct BindingAlignmentInfo
    {
        uint32_t binding;
        uint32_t structSize;
        std::vector<StructFieldInfo> fields;
    };

    class VertexAlignmentInfo
    {
    public:
        VertexAlignmentInfo(){}
        VertexAlignmentInfo(const std::vector<BindingAlignmentInfo>& cBindingAlignmentInfos);
        void create(const std::vector<BindingAlignmentInfo>& cBindingAlignmentInfos);
        VertexAlignmentInfo& operator=(const VertexAlignmentInfo& rInfo);
    private:
        friend class Window;
        const std::vector<BindingAlignmentInfo>& getAlignmentInfos() const;
        const uint32_t getIdentifier() const;
        static uint32_t count;
        uint32_t identifier;
        
        std::vector<BindingAlignmentInfo> bindingAlignmentInfos;
    };

    class VertexBuffer
    {
    public:
        VertexBuffer();
        VertexBuffer(const VertexBuffer& vb);
        VertexBuffer(const std::vector<uint32_t>& cVertexBufferBindings, const std::vector<uint32_t>& cVertexBufferSizes, const uint32_t cIndexBufferSize = 0);
        void create(const std::vector<uint32_t>& cVertexBufferBindings, const std::vector<uint32_t>& cVertexBufferSizes, const uint32_t cIndexBufferSize = 0);
        void setInstancingOptions(const uint32_t count, const uint32_t first);
        void updateVertexBuffer(const void * data, const uint32_t binding);           // TODO: make a staging transmission buffer a class field, make command buffer not one-time-submit buffer
        void updateIndexBuffer(const void * data);            // TODO: make a staging transmission buffer a class field, make command buffer not one-time-submit buffer
        VertexBuffer& operator=(const VertexBuffer& rBuffer);
        ~VertexBuffer();
    private:
        friend class Window;
        const vk::Buffer& getVertexBuffer(const uint32_t binding) const;
        const vk::Buffer& getIndexBuffer() const;
        const uint32_t getVertexBufferSize(const uint32_t binding) const;
        const uint32_t getIndexBufferSize() const;
        const vk::Fence* getIndexBufferFence() const;
        const vk::Fence* getVertexBufferFence(const uint32_t binding) const;
        const vk::Semaphore* getIndexBufferSemaphore() const;
        const vk::Semaphore* getVertexBufferSemaphore(const uint32_t binding) const;
        const uint32_t getInstanceCount() const;
        const uint32_t getFirstInstance() const;

        struct VertexBufferInfo
        {
            VertexBufferInfo(): size(0), memoryData(), updatedFence(), updatedSemaphore(), updateCommandBuffer(), buffer() {}
            VertexBufferInfo& operator=(const VertexBufferInfo rInfo)
            {
                size = rInfo.size;
                memoryData = rInfo.memoryData;
                updatedFence = rInfo.updatedFence;
                updatedSemaphore = rInfo.updatedSemaphore;
                updateCommandBuffer = rInfo.updateCommandBuffer;
                buffer = rInfo.buffer;
                return *this;
            }
            uint32_t size;
            system::AllocatedMemoryData memoryData;
            vk::Fence updatedFence;
            vk::Semaphore updatedSemaphore;
            vk::CommandBuffer updateCommandBuffer;
            //vk::Buffer buffer;
            utils::Buffer buffer;
        };

        void bindMemory();
        //std::vector<VertexAlignmentInfo> alignmentInfos;
        std::vector<uint32_t> vertexBufferBindings;
        std::vector<uint32_t> vertexBufferSizes;
        uint32_t indexBufferSize;
        system::AllocatedMemoryData indexMemoryData;
        std::map<uint32_t, VertexBufferInfo> vertexBuffers;
//        vk::Buffer indexBuffer;
        utils::Buffer indexBuffer;
        vk::Fence indexBufferUpdatedFence;
        vk::Semaphore indexBufferUpdatedSemaphore;
        vk::CommandBuffer indexUpdateCommandBuffer;
        uint32_t instanceCount;
        uint32_t firstInstance;
        bool transferred = false;

        void init();
        void update(const void * data, bool vertex, const uint32_t binding = 0);            // TODO: make a staging transmission buffer a class field, make command buffer not one-time-submit buffer
        void destroy();
    };

} 

#endif