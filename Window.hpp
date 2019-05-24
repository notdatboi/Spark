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
        Window(const uint32_t width, const uint32_t height, const std::string& title);
        Window(const uint32_t width, const uint32_t height, const std::string title);
        const vk::SurfaceKHR& getSurface() const;
        GLFWwindow* getWindow();
        void destroy();
        ~Window();
    private:
        GLFWwindow* window;
        vk::SurfaceKHR surface;
    };

}

#endif