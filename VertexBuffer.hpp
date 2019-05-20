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
        VertexBuffer(const VertexAlignmentInfo& cAlignmentInfo, const uint32_t cSize);
        void create(const VertexAlignmentInfo& cAlignmentInfo, const uint32_t cSize);
        void update(const vk::CommandBuffer& updateCommandBuffer, const void * data);           // TODO: make a staging transmission buffer a class field, make command buffer not one-timesubmit buffer
        VertexBuffer& operator=(const VertexBuffer& rBuffer);
        VertexBuffer& operator=(VertexBuffer& rBuffer);
        VertexBuffer& operator=(VertexBuffer&& rBuffer);
        ~VertexBuffer();
        /* */

        void bindMemory();
    private:
        //vk::Fence
        //vl::Semaphore
        VertexAlignmentInfo alignmentInfo;
        uint32_t size;
        AllocatedMemoryData memoryData;
        vk::Buffer buffer;
        vk::Fence bufferUpdatedFence;
        vk::Semaphore bufferUpdatedSemaphore;
        bool transferred = false;

        void init();
        void destroy();
    };

} 

#endif