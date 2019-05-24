#include"ShaderSet.hpp"

namespace spk
{
    ShaderSet::ShaderSet(){}

    ShaderSet::ShaderSet(const std::vector<ShaderInfo>& shaders)
    {
        create(shaders);
    }

    void ShaderSet::create(const std::vector<ShaderInfo>& shaders)
    {
        const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
        shaderModules.resize(shaders.size());
        for(int i = 0; i < shaders.size(); ++i)
        {
            vk::ShaderModuleCreateInfo info;
            std::vector<char> code = getCode(shaders[i].filename);
            info.setCodeSize(code.size());
            info.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

            logicalDevice.createShaderModule(&info, nullptr, &shaderModules[i].first);

            switch (shaders[i].type)
            {
                case ShaderType::vertex:
                    shaderModules[i].second = vk::ShaderStageFlagBits::eVertex;
                    break;
                case ShaderType::fragment:
                    shaderModules[i].second = vk::ShaderStageFlagBits::eFragment;
                    break;
            }
        }
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

    const std::vector<std::pair<vk::ShaderModule, vk::ShaderStageFlags> >& ShaderSet::getShaderModules() const
    {
        return shaderModules;
    }

    ShaderSet::~ShaderSet()
    {
        for(auto& module : shaderModules)
        {
            const vk::Device& logicalDevice = System::getInstance()->getLogicalDevice();
            logicalDevice.destroyShaderModule(module.first, nullptr);
        }
    }
}