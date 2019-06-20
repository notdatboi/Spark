#include"../include/ResourceSet.hpp"

namespace spk
{
    uint32_t ResourceSet::count = 0;

    ResourceSet::ResourceSet(): identifier(count)
    {
        ++count;
    }

    ResourceSet::ResourceSet(const ResourceSet& set): 
        identifier(count),
        textures(set.textures),
        uniformBuffers(set.uniformBuffers)
    {
        init();
        ++count;
    }

    ResourceSet& ResourceSet::operator=(const ResourceSet& set)
    {
        destroy();
        identifier = count;
        ++count;
        textures = set.textures;
        uniformBuffers = set.uniformBuffers;
        init();
        return *this;
    }

    ResourceSet::ResourceSet(std::vector<Texture>& cTextures, std::vector<UniformBuffer>& cUniformBuffers): 
        identifier(count),
        textures(cTextures), 
        uniformBuffers(cUniformBuffers)
    {
        init();
        ++count;
    }

    void ResourceSet::create(std::vector<Texture>& cTextures, std::vector<UniformBuffer>& cUniformBuffers)
    {
        textures.insert(textures.begin(), cTextures.begin(), cTextures.end());
        uniformBuffers.insert(uniformBuffers.begin(), cUniformBuffers.begin(), cUniformBuffers.end());
        init();
    }

    const vk::PipelineLayout& ResourceSet::getPipelineLayout() const 
    {
        return pipelineLayout;
    }

    const std::vector<vk::DescriptorSet>& ResourceSet::getDescriptorSets() const 
    {
        return descriptorSets;
    }

    const uint32_t ResourceSet::getIdentifier() const
    {
        return identifier;
    }

    void ResourceSet::update(const uint32_t set, const uint32_t binding, const void* data)
    {
        uint32_t index = setContainmentData[set].bindings[binding].first;
        if(setContainmentData[set].bindings[binding].second)
        {
            textures[index].update(data);
        }
        else
        {
            uniformBuffers[index].update(data);
        }
    }

    void ResourceSet::init()
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        const vk::CommandPool& commandPool = system::Executives::getInstance()->getPool();

        vk::CommandBufferAllocateInfo cbAllocInfo;
        cbAllocInfo.setCommandBufferCount(1);
        cbAllocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        cbAllocInfo.setCommandPool(commandPool);

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
        bindBufferMemory();
        createDescriptorPool();
        createDescriptorLayouts();
        allocateDescriptorSets();
        writeDescriptorData();
    }

    void ResourceSet::bindTextureMemory()
    {
        for(auto& texture : textures)
        {
            texture.bindMemory();
        }
    }

    void ResourceSet::bindBufferMemory()
    {
        for(auto& buffer : uniformBuffers)
        {
            buffer.bindMemory();
        }
    }

    void ResourceSet::createDescriptorPool()
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();

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
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
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
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        descriptorSets.resize(descriptorLayouts.size());
        vk::DescriptorSetAllocateInfo info;
        info.setDescriptorPool(descriptorPool);
        info.setDescriptorSetCount(descriptorLayouts.size());
        info.setPSetLayouts(descriptorLayouts.data());
        if(logicalDevice.allocateDescriptorSets(&info, descriptorSets.data()) != vk::Result::eSuccess) throw std::runtime_error("Failed to allocate descriptor sets!\n");
    }

    void ResourceSet::writeDescriptorData()
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        std::vector<vk::WriteDescriptorSet> setWrites;
        std::vector<vk::DescriptorImageInfo> imgInfos;
        std::vector<vk::DescriptorBufferInfo> bufInfos;
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
                    imgInfos.push_back(imgInfo);
                    write.setPBufferInfo(nullptr);
                }
                else
                {
                    bufInfo.setBuffer(uniformBuffers[binding.second.first].getBuffer());
                    bufInfo.setOffset(0);                                                       // because this is not memory offset, but buffer offset
                    bufInfo.setRange(uniformBuffers[binding.second.first].getSize());
                    bufInfos.push_back(bufInfo);
                    write.setPImageInfo(nullptr);
                }
                write.setPTexelBufferView(nullptr);
                setWrites.push_back(write);
            }
        }
        int i = 0, bufI = 0, imgI = 0;
        for(const auto& set : setContainmentData)
        {
            for(const auto& binding : set.second.bindings)
            {
                if(binding.second.second)
                {
                    setWrites[i].setPImageInfo(&imgInfos[imgI]);
                    ++imgI;
                }
                else
                {
                    setWrites[i].setPBufferInfo(&bufInfos[bufI]);
                    ++bufI;
                }
                ++i;
            }
        }
        logicalDevice.updateDescriptorSets(setWrites.size(), setWrites.data(), 0, nullptr);
    }

    void ResourceSet::destroy()
    {
        if(pipelineLayout)
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            const vk::CommandPool& commandPool = system::Executives::getInstance()->getPool();
            for(auto& layout : descriptorLayouts)
            {
                logicalDevice.destroyDescriptorSetLayout(layout, nullptr);
            }
            logicalDevice.destroyPipelineLayout(pipelineLayout, nullptr);
            pipelineLayout = vk::PipelineLayout();
            logicalDevice.destroyDescriptorPool(descriptorPool, nullptr);
            logicalDevice.destroySampler(uniqueSampler, nullptr);
        }
    }

    ResourceSet::~ResourceSet()
    {
        destroy();
    }

}