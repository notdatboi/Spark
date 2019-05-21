#ifndef SPARK_VERTEX_BUFFER_HPP
#define SPARK_VERTEX_BUFFER_HPP

#include"System.hpp"
#include"MemoryManager.hpp"
#include"Executives.hpp"
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

    struct VertexAlignmentInfo
    {
        uint32_t binding;
        uint32_t structSize;
        std::vector<StructFieldInfo> fields;
    };

    class VertexBuffer
    {
    public:
        /* Public interface */
        VertexBuffer();
        VertexBuffer(const VertexBuffer& vb);
        VertexBuffer(VertexBuffer&& vb);
        VertexBuffer(const VertexAlignmentInfo& cAlignmentInfo, const uint32_t cVertexBufferSize, const uint32_t cIndexBufferSize = 0);
        void create(const VertexAlignmentInfo& cAlignmentInfo, const uint32_t cVertexBufferSize, const uint32_t cIndexBufferSize = 0);
        void updateVertexBuffer(const void * data);           // TODO: make a staging transmission buffer a class field, make command buffer not one-time-submit buffer
        void updateIndexBuffer(const void * data);            // TODO: make a staging transmission buffer a class field, make command buffer not one-time-submit buffer
        VertexBuffer& operator=(const VertexBuffer& rBuffer);
        VertexBuffer& operator=(VertexBuffer& rBuffer);
        VertexBuffer& operator=(VertexBuffer&& rBuffer);
        ~VertexBuffer();
        /* */

        void bindMemory();
    private:
        //vk::Fence
        //vk::Semaphore
        VertexAlignmentInfo alignmentInfo;
        uint32_t vertexBufferSize;
        uint32_t indexBufferSize;
        AllocatedMemoryData vertexMemoryData;
        AllocatedMemoryData indexMemoryData;
        vk::Buffer vertexBuffer;
        vk::Buffer indexBuffer;
        vk::Fence vertexBufferUpdatedFence;
        vk::Semaphore vertexBufferUpdatedSemaphore;
        vk::Fence indexBufferUpdatedFence;
        vk::Semaphore indexBufferUpdatedSemaphore;
        vk::CommandBuffer vertexUpdateCommandBuffer;
        vk::CommandBuffer indexUpdateCommandBuffer;
        bool transferred = false;

        void init();
        void update(const void * data, bool vertex);            // TODO: make a staging transmission buffer a class field, make command buffer not one-time-submit buffer
        void destroy();
    };

} 

#endif