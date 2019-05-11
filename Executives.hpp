#ifndef SPARK_EXECUTIVES_HPP
#define SPARK_EXECUTIVES_HPP

#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define GLFW_INCLUDE_VULKAN

#include<vulkan/vulkan.hpp>

namespace spk
{

    class Executives
    {
    public:
        static Executives* getInstance();
        const uint32_t getGraphicsQueueFamilyIndex() const;
        const uint32_t getPresentQueueFamilyIndex() const;
        const vk::Queue& getGraphicsQueue() const;
        const vk::Queue& getPresentQueue() const;
        vk::Queue& getGraphicsQueue();
        vk::Queue& getPresentQueue();
        const vk::CommandPool& getPool() const;
        vk::CommandPool& getPool();
        void destroy();
    private:
        Executives();
        void createPool();

        static Executives* executivesInstance;
        uint32_t graphicsQueueFamilyIndex;
        uint32_t presentQueueFamilyIndex;
        vk::Queue graphicsQueue;
        vk::Queue presentQueue;
        vk::CommandPool pool;
    };

}

#endif