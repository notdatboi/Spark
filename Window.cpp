#include"Window.hpp"
#include"System.hpp"

namespace spk
{

    Window::Window(){}

    void Window::create(const uint32_t width, const uint32_t height, const std::string title)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);

        vk::Instance& instance = System::getInstance()->getvkInstance();
        VkSurfaceKHR tmpSurface;
        if(glfwCreateWindowSurface(instance, window, nullptr, &tmpSurface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!\n");
        }
        surface = tmpSurface;

        presentQueue = Executives::getInstance()->getPresentQueue(surface);
    }

    Window::Window(const uint32_t width, const uint32_t height, const std::string title)
    {
        create(width, height, title);
    }
    
    const vk::SurfaceKHR& Window::getSurface() const
    {
        return surface;
    }

    GLFWwindow* Window::getWindow()
    {
        return window;
    }

    std::pair<uint32_t, const vk::Queue*> Window::getPresentQueue()
    {
        return presentQueue;
    }

    void Window::destroy()
    {
        const auto& instance = System::getInstance()->getvkInstance();
        instance.destroySurfaceKHR(surface, nullptr);
        glfwDestroyWindow(window);
    }

    Window::~Window()
    {
        destroy();
    }

}