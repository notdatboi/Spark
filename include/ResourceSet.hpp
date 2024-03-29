#ifndef SPARK_RESOURCE_SET_HPP
#define SPARK_RESOURCE_SET_HPP

#include"Texture.hpp"
#include"UniformBuffer.hpp"
#include<map>
#include"SparkIncludeBase.hpp"

namespace spk
{

    class ResourceSet
    {
    public:
        ResourceSet();
        ResourceSet(std::vector<Texture>& cTextures, std::vector<UniformBuffer>& cUniformBuffers);
        ResourceSet(const ResourceSet& set);
        ResourceSet& operator=(const ResourceSet& set);
        void create(std::vector<Texture>& cTextures, std::vector<UniformBuffer>& cUniformBuffers);
        void update(const uint32_t set, const uint32_t binding, const void* data);
        ~ResourceSet();
    private:
        struct ResourceSetContainmentInfo
        {
            std::map<uint32_t, std::pair<uint32_t, bool> > bindings; // [binding]: {resourceVectorIndex, texture or buffer (t or f)}
        };

        friend class Window;
        const vk::PipelineLayout& getPipelineLayout() const;
        const std::vector<vk::DescriptorSet>& getDescriptorSets() const;
        const uint32_t getIdentifier() const;

        std::vector<Texture> textures;
        std::vector<UniformBuffer> uniformBuffers;
        vk::DescriptorPool descriptorPool;
        vk::Sampler uniqueSampler;
        std::map<uint32_t, ResourceSetContainmentInfo> setContainmentData; // [setIndex]: {bindings}
        std::vector<vk::DescriptorSet> descriptorSets;
        std::vector<vk::DescriptorSetLayout> descriptorLayouts;
        vk::PipelineLayout pipelineLayout;
        static uint32_t count;
        uint32_t identifier;

        void init();
        void bindTextureMemory();
        void bindBufferMemory();
        void createDescriptorPool();
        void createDescriptorLayouts();
        void allocateDescriptorSets();
        void writeDescriptorData();
        void destroy();
    };

}

#endif