#include"System.hpp"
#include"Executives.hpp"
#include"WindowSystem.hpp"

namespace spk
{

    System::System()
    {
        glfwInit();
    }

    vk::Instance& System::getvkInstance()
    {
        return instance;
    }

    vk::PhysicalDevice& System::getPhysicalDevice()
    {
        return physicalDevice;
    }

    vk::Device& System::getLogicalDevice()
    {
        return logicalDevice;
    }

    const vk::Instance& System::getvkInstance() const
    {
        return instance;
    }

    const vk::PhysicalDevice& System::getPhysicalDevice() const
    {
        return physicalDevice;
    }

    const vk::Device& System::getLogicalDevice() const
    {
        return logicalDevice;
    }

    std::unique_ptr<System> System::systemInstance = nullptr;

    System* System::getInstance()
    {
        static bool created = false;
        if(!created)
        {
            systemInstance.reset(new System());
            created = true;
            systemInstance->createInstance();
            WindowSystem::getInstance();
            systemInstance->createPhysicalDevice();
            Executives::getInstance();
            systemInstance->createLogicalDevice();
            Executives::getInstance();
        }
        return systemInstance.get();
    }

    std::vector<const char*> System::getInstanceExtensions() const
    {
        uint32_t glfwExtCount = 1;
        const char ** glfwExtData;
        glfwExtData = glfwGetRequiredInstanceExtensions(&glfwExtCount);
        std::vector<const char *> extData(glfwExtData, glfwExtData + glfwExtCount);
        if(enableValidation) extData.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        return extData;
    }

    std::vector<const char *> System::getDeviceExtensions() const
    {
        std::vector<const char *> neededExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        std::vector<const char *> result;
        uint32_t deviceExtPropertyCount;
        physicalDevice.enumerateDeviceExtensionProperties(nullptr, &deviceExtPropertyCount, nullptr);
        std::vector<vk::ExtensionProperties> deviceExtProperties(deviceExtPropertyCount);
        physicalDevice.enumerateDeviceExtensionProperties(nullptr, &deviceExtPropertyCount, deviceExtProperties.data());
        for(const auto& ext : neededExtensions)
        {
            for(const auto& property : deviceExtProperties)
            {
                if(std::string(property.extensionName) == std::string(ext)) result.push_back(ext);
            }
        }
        if(result.size() != neededExtensions.size()) throw std::runtime_error("Failed to set up extensions!\n");
        return result;
    }

    std::vector<const char*> System::getInstanceLayers() const
    {
        if(enableValidation)
        {
            const char * preferredValidationLayer = "VK_LAYER_LUNARG_standard_validation";
            uint32_t layerCount;
            vk::enumerateInstanceLayerProperties(&layerCount, nullptr);
            std::vector<vk::LayerProperties> layerData(layerCount);
            vk::enumerateInstanceLayerProperties(&layerCount, layerData.data());
            for(auto& p : layerData)
            {
                if(std::string(p.layerName) == std::string(preferredValidationLayer))
                {
                    std::vector<const char*> result(1, preferredValidationLayer);
                    return result;
                }
            }
        }
        throw std::runtime_error("Can't enable validation layer!\n");
    }

    void System::createInstance()
    {
        std::vector<const char*> instanceExtensions = getInstanceExtensions();
        vk::InstanceCreateInfo instanceInfo;
        instanceInfo.setEnabledExtensionCount(instanceExtensions.size());
        instanceInfo.setPpEnabledExtensionNames(instanceExtensions.data());
        std::vector<const char*> instanceLayers = getInstanceLayers();
        instanceInfo.setEnabledLayerCount(instanceLayers.size());
        instanceInfo.setPpEnabledLayerNames(instanceLayers.data());
        vk::ApplicationInfo appInfo;
        appInfo.setApiVersion(VK_MAKE_VERSION(1, 0, 0));
        appInfo.setApplicationVersion(VK_MAKE_VERSION(0, 0, 1));
        appInfo.setPApplicationName("TEST");
        instanceInfo.setPApplicationInfo(&appInfo);
        if(vk::createInstance(&instanceInfo, nullptr, &instance) != vk::Result::eSuccess) throw std::runtime_error("Failed to create instance!\n");
    }

    void System::createPhysicalDevice()
    {
        uint32_t physDeviceCount;
        instance.enumeratePhysicalDevices(&physDeviceCount, nullptr);
        std::vector<vk::PhysicalDevice> devicesData(physDeviceCount);
        instance.enumeratePhysicalDevices(&physDeviceCount, devicesData.data());
        bool preferredDevicePicked = false;
        for(auto& device : devicesData)
        {
            vk::PhysicalDeviceProperties deviceProperties;
            device.getProperties(&deviceProperties);
            if(deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                preferredDevicePicked = true;
                physicalDevice = device;
            }
        }
        if(!preferredDevicePicked) physicalDevice = devicesData[0];
    }

    void System::createLogicalDevice()
    {
        vk::DeviceCreateInfo logicalDeviceCreateInfo;
        logicalDeviceCreateInfo.setEnabledLayerCount(0);
        logicalDeviceCreateInfo.setPpEnabledLayerNames(nullptr);
        std::vector<const char *> deviceExtensions = getDeviceExtensions();
        logicalDeviceCreateInfo.setEnabledExtensionCount(deviceExtensions.size());
        logicalDeviceCreateInfo.setPpEnabledExtensionNames(deviceExtensions.data());
        uint32_t presentQueueFamilyIndex = Executives::getInstance()->getPresentQueueFamilyIndex();
        uint32_t graphicsQueueFamilyIndex = Executives::getInstance()->getGraphicsQueueFamilyIndex();
        const float priorities[] = {0.5};
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        if(presentQueueFamilyIndex == graphicsQueueFamilyIndex)
        {
            logicalDeviceCreateInfo.setQueueCreateInfoCount(1);
            vk::DeviceQueueCreateInfo queueCreateInfo;
            queueCreateInfo.setQueueFamilyIndex(presentQueueFamilyIndex);
            queueCreateInfo.setQueueCount(1);
            queueCreateInfo.setPQueuePriorities(priorities);
            queueCreateInfos.push_back(queueCreateInfo);
        }
        else
        {
            logicalDeviceCreateInfo.setQueueCreateInfoCount(2);
            queueCreateInfos = std::vector<vk::DeviceQueueCreateInfo>(2);
            queueCreateInfos[0].setQueueFamilyIndex(graphicsQueueFamilyIndex);
            queueCreateInfos[1].setQueueFamilyIndex(presentQueueFamilyIndex);
            queueCreateInfos[0].setQueueCount(1);
            queueCreateInfos[1].setQueueCount(1);
            queueCreateInfos[0].setPQueuePriorities(priorities);
            queueCreateInfos[1].setPQueuePriorities(priorities);
        }
        logicalDeviceCreateInfo.setPQueueCreateInfos(queueCreateInfos.data());
        vk::PhysicalDeviceFeatures deviceFeatures;
        // deviceFeatures.setGeometryShader(true);
        deviceFeatures.setTessellationShader(true);
        // physicalDeviceFeatures
        logicalDeviceCreateInfo.setPEnabledFeatures(&deviceFeatures);
        if(physicalDevice.createDevice(&logicalDeviceCreateInfo, nullptr, &logicalDevice) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to create logical device!\n");
        }

        PFN_vkGetInstanceProcAddr getInstanceProcAddr = PFN_vkGetInstanceProcAddr(instance.getProcAddr("vkGetInstanceProcAddr"));
        PFN_vkGetDeviceProcAddr getDeviceProcAddr = PFN_vkGetDeviceProcAddr(logicalDevice.getProcAddr("vkGetDeviceProcAddr"));
        if(getInstanceProcAddr == nullptr || getDeviceProcAddr == nullptr) throw std::runtime_error("Failed to get instance or device process addresses!\n");
        loader.init(instance, getInstanceProcAddr, logicalDevice, getDeviceProcAddr);

        if(enableValidation)
        {
            vk::DebugUtilsMessengerCreateInfoEXT messengerInfo;
            messengerInfo.setMessageSeverity(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | 
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | 
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose);
            messengerInfo.setMessageType(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
            messengerInfo.setPfnUserCallback(callback);
            messengerInfo.setPUserData(nullptr);
            if(instance.createDebugUtilsMessengerEXT(&messengerInfo, nullptr, &debugMessenger, loader) != vk::Result::eSuccess) throw std::runtime_error("Failed to create debug messenger!\n");
        }
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL System::callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        std::cout << "DEBUG INFO: " << pCallbackData->pMessage << '\n';
        return false;
    }

    void System::destroy()
    {
        spk::Executives::getInstance()->destroy();
        spk::MemoryManager::getInstance()->destroy();
        spk::WindowSystem::getInstance()->destroy();
        if(enableValidation)
        {
            instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, loader);
        }
        logicalDevice.destroy(nullptr);
        instance.destroy(nullptr);
        glfwTerminate();
    }

}