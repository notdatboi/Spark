#ifndef SPARK_WINDOW_SYSTEM_HPP
#define SPARK_WINDOW_SYSTEM_HPP

#include"SparkIncludeBase.hpp"
#include<memory>
#include"ResourceSet.hpp"

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