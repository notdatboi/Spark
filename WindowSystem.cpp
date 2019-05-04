#include"WindowSystem.hpp"
#include"System.hpp"

namespace spk
{

    WindowSystem* WindowSystem::windowSystemInstance = nullptr;

    WindowSystem::WindowSystem()
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(800, 600, "AAA", nullptr, nullptr);

        vk::Instance& instance = System::getInstance()->getvkInstance();
        VkSurfaceKHR tmpSurface;
        if(glfwCreateWindowSurface(instance, window, nullptr, &tmpSurface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!\n");
        }
        surface = tmpSurface;
    }
    
    vk::SurfaceKHR& WindowSystem::getSurface()
    {
        return surface;
    }

    const vk::SurfaceKHR& WindowSystem::getSurface() const
    {
        return surface;
    }

    WindowSystem* WindowSystem::getInstance()
    {
        static bool created = false;
        if(!created)
        {
            std::cout << "Creating window system\n";
            windowSystemInstance = new WindowSystem();
            created = true;
        }
        return windowSystemInstance;
    }

    WindowSystem::~WindowSystem()
    {
        glfwDestroyWindow(window);
    }

}