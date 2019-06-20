#include"../include/ShaderSet.hpp"

namespace spk
{
    uint32_t ShaderSet::count = 0;

    ShaderSet::ShaderSet(): identifier(count)
    {
        ++count;
    }

    ShaderSet::ShaderSet(const ShaderSet& set): identifier(count)
    {
        create(set.infos);
        ++count;
    }

    ShaderSet::ShaderSet(const std::vector<ShaderInfo>& shaders): identifier(count)
    {
        create(shaders);
        ++count;
    }

    const uint32_t ShaderSet::getIdentifier() const
    {
        return identifier;
    }

    ShaderSet& ShaderSet::operator=(const ShaderSet& set)
    {
        destroy();
        create(set.infos);
        identifier = count;
        ++count;
        return *this;
    }

    void ShaderSet::create(const std::vector<ShaderInfo>& shaders)
    {
        const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
        infos = shaders;
        shaderModules.resize(shaders.size());
        shaderStages.resize(shaders.size());
        for(int i = 0; i < shaders.size(); ++i)
        {
            vk::ShaderModuleCreateInfo info;
            std::vector<char> code = getCode(shaders[i].filename);
            info.setCodeSize(code.size());
            info.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

            logicalDevice.createShaderModule(&info, nullptr, &shaderModules[i].first);

            switch (shaders[i].type)
            {
                case ShaderType::Vertex:
                    shaderModules[i].second = vk::ShaderStageFlagBits::eVertex;
                    break;
                case ShaderType::Fragment:
                    shaderModules[i].second = vk::ShaderStageFlagBits::eFragment;
                    break;
            }
        }
        for(int i = 0; i < shaderModules.size(); ++i)
        {
            shaderStages[i].setModule(shaderModules[i].first);
            shaderStages[i].setStage(shaderModules[i].second);
            shaderStages[i].setPName("main");
            shaderStages[i].setPSpecializationInfo(nullptr);
        }
    }

    const std::vector<vk::PipelineShaderStageCreateInfo>& ShaderSet::getShaderStages() const
    {
        return shaderStages;
    }

    std::vector<char> ShaderSet::getCode(const std::string& filename) const
    {
        std::ifstream fin;
        fin.open(filename, std::ios::ate | std::ios::binary);
        size_t size = fin.tellg();
        std::vector<char> code(size);
        fin.seekg(0);
        fin.read(code.data(), size);
        fin.close();
        return code;
    }

    void ShaderSet::destroy()
    {
        if(shaderModules.size() != 0 && shaderModules[0].first)
        {
            for(auto& module : shaderModules)
            {
                const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
                logicalDevice.destroyShaderModule(module.first, nullptr);
            }
        }
    }

    ShaderSet::~ShaderSet()
    {
        destroy();
    }
}