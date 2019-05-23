#ifndef SPARK_EXECUTIVES_HPP
#define SPARK_EXECUTIVES_HPP

#include"SparkIncludeBase.hpp"
#include<memory>
#include<map>

namespace spk
{

    class Executives
    {
    public:
        static Executives* getInstance();
        const uint32_t getGraphicsQueueFamilyIndex() const;
        const vk::Queue& getGraphicsQueue() const;
        const vk::CommandPool& getPool() const;
        std::pair<uint32_t, const vk::Queue&> getPresentQueue(const vk::SurfaceKHR& surface) const;
        void destroy();
    private:
        Executives();
        void createPool();

        std::vector<vk::QueueFamilyProperties> queueFamilyProperties;
        static std::unique_ptr<Executives> executivesInstance;
        uint32_t graphicsQueueFamilyIndex;
        std::map<uint32_t, vk::Queue> presentQueues;        // key = family index, value = present queue
        vk::Queue graphicsQueue;
        vk::CommandPool pool;
    };

}

#endif