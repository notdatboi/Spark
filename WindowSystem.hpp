#ifndef SPARK_WINDOW_SYSTEM_HPP
#define SPARK_WINDOW_SYSTEM_HPP

//#include {RenderTargetName.hpp}

namespace spk
{

    class WindowSystem
    {
    public:
        static WindowSystem* getInstance();
        operator vk::SurfaceKHR() const;
        ~WindowSystem();
    private:
        WindowSystem();

        static WindowSystem* windowSystemInstance;
        GLFWwindow* window;
        vk::SurfaceKHR surface;
    };

}

#endif