#include"ResourceSet.hpp"

namespace spk
{

    ResourceSet::ResourceSet(){}

    ResourceSet::ResourceSet(std::vector<Texture>& cTextures, std::vector<UniformBuffer>& cUniformBuffers): 
        textures(std::move(cTextures)), 
        uniformBuffers(std::move(cUniformBuffers))
    {
        init();
    }

    void ResourceSet::update(const uint32_t set, const uint32_t binding, const void* data)
    {
        if(setContainmentData[set].bindings[binding].second) throw std::runtime_error("You can't change textures.\n");
        else
        {
            uint32_t index = setContainmentData[set].bindings[binding].first;
            uniformBuffers[index].update(data);
        }
    }

    void ResourceSet::init()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        const vk::CommandPool& commandPool = Executives::getInstance()->getPool();

        vk::CommandBufferAllocateInfo cbAllocInfo;
        cbAllocInfo.setCommandBufferCount(1);
        cbAllocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        cbAllocInfo.setCommandPool(commandPool);
        if(logicalDevice.allocateCommandBuffers(&cbAllocInfo, &initialCommandBuffer) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate command buffer!\n");

        vk::SamplerCreateInfo samplerInfo;
        samplerInfo.setMagFilter(vk::Filter::eLinear);
        samplerInfo.setMinFilter(vk::Filter::eNearest);
        samplerInfo.setMipmapMode(vk::SamplerMipmapMode::eNearest);
        samplerInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat);
        samplerInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat);
        samplerInfo.setAddressModeW(vk::SamplerAddressMode::eRepeat);
        samplerInfo.setMipLodBias(0);
        samplerInfo.setAnisotropyEnable(false);
        samplerInfo.setCompareEnable(false);
        samplerInfo.setMinLod(0);
        samplerInfo.setMaxLod(1);
        samplerInfo.setBorderColor(vk::BorderColor::eFloatTransparentBlack);
        samplerInfo.setUnnormalizedCoordinates(false);

        if(logicalDevice.createSampler(&samplerInfo, nullptr, &uniqueSampler) != vk::Result::eSuccess) throw std::runtime_error("Failed to create sampler!\n");

        if(textures.size() != 0 || uniformBuffers.size() != 0)
        {
            uint32_t index = 0;
            for(const auto& texture : textures)
            {
                uint32_t set = texture.getSet();
                uint32_t binding = texture.getBinding();
                if(setContainmentData.count(set) == 0)
                {
                    setContainmentData[set].bindings[binding] = std::make_pair(index, true);
                }
                else
                {
                    if(setContainmentData[set].bindings.count(binding) == 0 && setContainmentData[set].bindings.begin()->second.second)
                    {
                        setContainmentData[set].bindings[binding] = std::make_pair(index, true);
                    }
                    else throw std::runtime_error("Binding already exists or the set is used by the other type of resource!\n");
                }
                ++index;
            }
            index = 0;
            for(const auto& buffer : uniformBuffers)
            {
                uint32_t set = buffer.getSet();
                uint32_t binding = buffer.getBinding();
                if(setContainmentData.count(set) == 0)
                {
                    setContainmentData[set].bindings[binding] = std::make_pair(index, false);
                }
                else
                {
                    if(setContainmentData[set].bindings.count(binding) == 0 && !setContainmentData[set].bindings.begin()->second.second)
                    {
                        setContainmentData[set].bindings[binding] = std::make_pair(index, false);
                    }
                    else throw std::runtime_error("Binding already exists or the set is used by the other type of resource!\n");
                }
                ++index;
            }
            index = 0;
            for(auto& set : setContainmentData)
            {
                if(set.first != index) throw std::runtime_error("Non-consecutive sets!\n");
                ++index;
            }
        }

        bindTextureMemory();
        createDescriptorPool();
        createDescriptorLayouts();
        allocateDescriptorSets();
        writeDescriptorData();
    }

    void ResourceSet::bindTextureMemory()
    {
        for(auto& texture : textures)
        {
            texture.bindMemory(initialCommandBuffer);
        }
    }

    void ResourceSet::createDescriptorPool()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();

        vk::DescriptorPoolSize textureSize;
        textureSize.setType(vk::DescriptorType::eCombinedImageSampler);
        textureSize.setDescriptorCount(textures.size());
        vk::DescriptorPoolSize uniformBufferSize;
        uniformBufferSize.setType(vk::DescriptorType::eUniformBuffer);
        uniformBufferSize.setDescriptorCount(uniformBuffers.size());
        vk::DescriptorPoolSize poolSizes[] = {textureSize, uniformBufferSize};

        vk::DescriptorPoolCreateInfo poolInfo;
        poolInfo.setFlags(vk::DescriptorPoolCreateFlags());     // no individual reset
        poolInfo.setMaxSets(setContainmentData.size());
        poolInfo.setPoolSizeCount(2);
        poolInfo.setPPoolSizes(poolSizes);

        if(logicalDevice.createDescriptorPool(&poolInfo, nullptr, &descriptorPool) != vk::Result::eSuccess) throw std::runtime_error("Failed to create descriptor pool!\n");
    }

    void ResourceSet::createDescriptorLayouts()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        descriptorLayouts.resize(setContainmentData.size());
        size_t setIndex = 0;
        for(auto& set : setContainmentData)
        {
            std::vector<vk::DescriptorSetLayoutBinding> bindings(set.second.bindings.size());
            size_t index = 0;
            vk::DescriptorType descType = set.second.bindings.begin()->second.second ? vk::DescriptorType::eCombinedImageSampler : vk::DescriptorType::eUniformBuffer;
            for(auto& binding : set.second.bindings)
            {
                bindings[index].setBinding(binding.first);
                bindings[index].setDescriptorType(descType);
                bindings[index].setDescriptorCount(1);
                bindings[index].setStageFlags(vk::ShaderStageFlagBits::eAllGraphics);       // TODO: synchronize it with shader modules of pipeline
                bindings[index].setPImmutableSamplers(nullptr);
                ++index;
            }
            vk::DescriptorSetLayoutCreateInfo layoutInfo;
            layoutInfo.setBindingCount(bindings.size());
            layoutInfo.setPBindings(bindings.data());
            if(logicalDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorLayouts[setIndex]) != vk::Result::eSuccess) throw std::runtime_error("Failed to create descriptor layout!\n");

            ++setIndex;
        }

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayoutInfo.setSetLayoutCount(descriptorLayouts.size());
        pipelineLayoutInfo.setPSetLayouts(descriptorLayouts.data());
        pipelineLayoutInfo.setPushConstantRangeCount(0);
        pipelineLayoutInfo.setPPushConstantRanges(nullptr);
        if(logicalDevice.createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) != vk::Result::eSuccess) throw std::runtime_error("Failed to create pipeline layout!\n");
    }

    void ResourceSet::allocateDescriptorSets()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        descriptorSets.resize(descriptorLayouts.size());
        vk::DescriptorSetAllocateInfo info;
        info.setDescriptorPool(descriptorPool);
        info.setDescriptorSetCount(descriptorLayouts.size());
        info.setPSetLayouts(descriptorLayouts.data());
        if(logicalDevice.allocateDescriptorSets(&info, descriptorSets.data()) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate descriptor sets!\n");
    }

    void ResourceSet::writeDescriptorData()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        std::vector<vk::WriteDescriptorSet> setWrites;
        for(const auto& set : setContainmentData)
        {
            for(const auto& binding : set.second.bindings)
            {
                vk::WriteDescriptorSet write;
                write.setDstSet(descriptorSets[set.first]);
                write.setDstBinding(binding.first);
                write.setDstArrayElement(0);
                write.setDescriptorCount(1);
                write.setDescriptorType(binding.second.second ? vk::DescriptorType::eCombinedImageSampler : vk::DescriptorType::eUniformBuffer);
                vk::DescriptorImageInfo imgInfo;
                vk::DescriptorBufferInfo bufInfo;
                if(binding.second.second)
                {
                    imgInfo.setImageLayout(textures[binding.second.first].getLayout());
                    imgInfo.setSampler(uniqueSampler);
                    imgInfo.setImageView(textures[binding.second.first].getImageView());
                    write.setPImageInfo(&imgInfo);
                    write.setPBufferInfo(nullptr);
                }
                else
                {
                    bufInfo.setBuffer(uniformBuffers[binding.second.first].getBuffer());
                    bufInfo.setOffset(uniformBuffers[binding.second.first].getOffset());
                    bufInfo.setRange(uniformBuffers[binding.second.first].getSize());
                    write.setPBufferInfo(&bufInfo);
                    write.setPImageInfo(nullptr);
                }
                write.setPTexelBufferView(nullptr);
                setWrites.push_back(write);
            }
        }
        logicalDevice.updateDescriptorSets(setWrites.size(), setWrites.data(), 0, nullptr);
    }

    ResourceSet::~ResourceSet()
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        const vk::CommandPool& commandPool = Executives::getInstance()->getPool();
        logicalDevice.freeCommandBuffers(commandPool, 1, &initialCommandBuffer);
        for(auto& layout : descriptorLayouts)
        {
            logicalDevice.destroyDescriptorSetLayout(layout, nullptr);
        }
        logicalDevice.destroyPipelineLayout(pipelineLayout, nullptr);
        logicalDevice.destroyDescriptorPool(descriptorPool, nullptr);
        logicalDevice.destroySampler(uniqueSampler, nullptr);
    }

}