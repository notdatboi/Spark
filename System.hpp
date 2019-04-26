#ifndef TEST_SYSTEM_HPP
#define TEST_SYSTEM_HPP

#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define GLFW_INCLUDE_VULKAN

#include<vulkan/vulkan.hpp>
#include<iostream>
#include<vector>
#include<string>
#include<GLFW/glfw3.h>
#include<stdexcept>

#define DEBUG

namespace spk
{

    #ifdef DEBUG
    const bool enableValidation = true;
    #else
    const bool enableValidation = false;
    #endif

    class System
    {
    public:
        static System* getInstance();
        //operator vk::Instance() const;
        //operator vk::PhysicalDevice() const;
        //operator vk::Device() const;
        vk::Instance& getvkInstance();
        vk::Device& getLogicalDevice();
        vk::PhysicalDevice& getPhysicalDevice();
        const vk::Instance& getvkInstance() const;
        const vk::Device& getLogicalDevice() const;
        const vk::PhysicalDevice& getPhysicalDevice() const;
        ~System();
    private:
        System();
        std::vector<const char*> getInstanceExtensions() const;
        std::vector<const char*> getDeviceExtensions() const;
        std::vector<const char*> getInstanceLayers() const;
        void createInstance();
        void createPhysicalDevice();
        void createLogicalDevice();

        static System* systemInstance;
        vk::Instance instance;
        vk::PhysicalDevice physicalDevice;
        vk::Device logicalDevice;
    };

}

#endif