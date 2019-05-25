#ifndef SPARK_WINDOW_SYSTEM_HPP
#define SPARK_WINDOW_SYSTEM_HPP

#include"SparkIncludeBase.hpp"
#include<memory>
#include<string>
#include"ResourceSet.hpp"
#include"ShaderSet.hpp"

namespace spk
{

    class Window
    {
    public:
        Window();
        void create(const uint32_t cWidth, const uint32_t cHeight, const std::string title);
        Window(const uint32_t cWidth, const uint32_t cHeight, const std::string title);
        void destroy();
        ~Window();

        const vk::SurfaceKHR& getSurface() const;
        GLFWwindow* getWindow();
        std::pair<uint32_t, const vk::Queue*> getPresentQueue();
    private:
        GLFWwindow* window;
        vk::SurfaceKHR surface;
        std::pair<uint32_t, const vk::Queue*> presentQueue;
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> swapchainImages;
        std::vector<vk::ImageView> swapchainImageViews;
        vk::RenderPass renderPass;
        vk::SurfaceFormatKHR surfaceFormat;
        std::vector<vk::Framebuffer> framebuffers;

        uint32_t width;
        uint32_t height;

        void createSwapchain();
        void createRenderPass();
        void createFramebuffers();
    };

}

#endif