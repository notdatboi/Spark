#ifndef SPARK_SYSTEM_HPP
#define SPARK_SYSTEM_HPP

#include"SparkIncludeBase.hpp"
#include<vector>
#include<string>
#include<iostream>
#include<memory>

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
        /* Public interface */
        static System* getInstance();
        /* */

        //operator vk::Instance() const;
        //operator vk::PhysicalDevice() const;
        //operator vk::Device() const;
        vk::Instance& getvkInstance();
        vk::Device& getLogicalDevice();
        vk::PhysicalDevice& getPhysicalDevice();
        const vk::Instance& getvkInstance() const;
        const vk::Device& getLogicalDevice() const;
        const vk::PhysicalDevice& getPhysicalDevice() const;
        void destroy();
    private:
        System();
        std::vector<const char*> getInstanceExtensions() const;
        std::vector<const char*> getDeviceExtensions() const;
        std::vector<const char*> getInstanceLayers() const;
        void createInstance();
        void createPhysicalDevice();
        void createLogicalDevice();
        static VKAPI_ATTR VkBool32 VKAPI_CALL callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

        static std::unique_ptr<System> systemInstance;
        vk::Instance instance;
        vk::PhysicalDevice physicalDevice;
        vk::Device logicalDevice;
        vk::DispatchLoaderDynamic loader;
        vk::DebugUtilsMessengerEXT debugMessenger;
    };

}

#endif