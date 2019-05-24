#ifndef SPARK_SHADER_SET_HPP
#define SPARK_SHADER_SET_HPP

#include"SparkIncludeBase.hpp"
#include<vector>
#include<string>
#include<fstream>
#include"System.hpp"

namespace spk
{
    enum class ShaderType
    {
        vertex, 
        fragment
    };

    struct ShaderInfo
    {
        ShaderType type;
        std::string filename;
    };

    class ShaderSet
    {
    public:
        ShaderSet();
        ShaderSet(const std::vector<ShaderInfo>& shaders);
        void create(const std::vector<ShaderInfo>& shaders);
        ~ShaderSet();

        const std::vector<std::pair<vk::ShaderModule, vk::ShaderStageFlags> >& getShaderModules() const;
    private:

        std::vector<char> getCode(const std::string& filename) const;
        std::vector<std::pair<vk::ShaderModule, vk::ShaderStageFlags> > shaderModules;
    };
}

#endif