#ifndef SPARK_WINDOW_SYSTEM_HPP
#define SPARK_WINDOW_SYSTEM_HPP

#include"SparkIncludeBase.hpp"
#include<memory>
#include<string>
#include"ResourceSet.hpp"

namespace spk
{

    class Window
    {
    public:
        Window();
        void create(const uint32_t width, const uint32_t height, const std::string title);
        Window(const uint32_t width, const uint32_t height, const std::string title);
        void destroy();
        ~Window();

        const vk::SurfaceKHR& getSurface() const;
        GLFWwindow* getWindow();
        std::pair<uint32_t, const vk::Queue*> getPresentQueue();
    private:
        GLFWwindow* window;
        vk::SurfaceKHR surface;
        std::pair<uint32_t, const vk::Queue*> presentQueue;
    };

}

#endif