#ifndef SPARK_WINDOW_SYSTEM_HPP
#define SPARK_WINDOW_SYSTEM_HPP

#include"ResourceSet.hpp"
#include<vulkan/vulkan.hpp>
#include<GLFW/glfw3.h>
#include<memory>

namespace spk
{

    class WindowSystem
    {
    public:
        static WindowSystem* getInstance();
        vk::SurfaceKHR& getSurface();
        const vk::SurfaceKHR& getSurface() const;
        GLFWwindow* getWindow();
        void destroy();
    private:
        WindowSystem();

        static std::unique_ptr<WindowSystem> windowSystemInstance;
        GLFWwindow* window;
        vk::SurfaceKHR surface;
    };

}

#endif