#ifndef SPARK_RESOURCE_SET_HPP
#define SPARK_RESOURCE_SET_HPP

#include"Texture.hpp"
#include"UniformBuffer.hpp"
#include<map>

namespace spk
{

    struct ResourceSetContainmentInfo
    {
        std::map<uint32_t, std::pair<uint32_t, bool> > bindings; // [binding]: {resourceVectorIndex, texture or buffer (t or f)}
    };

    class ResourceSet
    {
    public:
        ResourceSet();
        ResourceSet(std::vector<Texture>& cTextures, std::vector<UniformBuffer>& cUniformBuffers);
        void create(std::vector<Texture>& cTextures, std::vector<UniformBuffer>& cUniformBuffers);
        void update(uint32_t set, uint32_t binding, const void* data);  // TODO: implement
        ~ResourceSet();
    private:
        vk::CommandBuffer initialCommandBuffer;
        std::vector<Texture> textures;
        std::vector<UniformBuffer> uniformBuffers;
        vk::DescriptorPool descriptorPool;
        vk::Sampler uniqueSampler;
        std::map<uint32_t, ResourceSetContainmentInfo> setContainmentData; // [setIndex]: {bindings}
        std::vector<vk::DescriptorSet> descriptorSets;
        std::vector<vk::DescriptorSetLayout> descriptorLayouts;
        vk::PipelineLayout pipelineLayout;

        void init();
        void bindTextureMemory();
        void createDescriptorPool();
        void createDescriptorLayouts();
        void allocateDescriptorSets();
        void writeDescriptorData();
    };

}

#endif