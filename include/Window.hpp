#ifndef SPARK_WINDOW_SYSTEM_HPP
#define SPARK_WINDOW_SYSTEM_HPP

#include"SparkIncludeBase.hpp"
#include<memory>
#include<string>
#include<map>
#include<tuple>
#include"ResourceSet.hpp"
#include"VertexBuffer.hpp"
#include"ShaderSet.hpp"

namespace spk
{
    enum class CullMode
    {
        Clockwise,
        CounterClockwise,
        None
    };

    struct DrawOptions
    {
        CullMode cullMode;
    };

    class Window
    {
    public:
        Window();
        void create(const uint32_t cWidth, const uint32_t cHeight, const std::string cTitle, const DrawOptions cOptions);
        Window(const uint32_t cWidth, const uint32_t cHeight, const std::string cTitle, const DrawOptions cOptions);
        void destroy();
        void draw(const ResourceSet* resources, const VertexAlignmentInfo* alignmentInfo, const std::vector<VertexBuffer*>& vertexBuffers, const ShaderSet* shaders);
        ~Window();
        GLFWwindow* getGLFWWindow();
    private:
        struct DrawComponents
        {
            vk::Pipeline pipeline;
            const ResourceSet* resources;
            const VertexAlignmentInfo* alignmentInfo;
            const ShaderSet* shaders;
        };
        GLFWwindow* window;
        vk::SurfaceKHR surface;
        std::pair<uint32_t, const vk::Queue*> presentQueue;
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> swapchainImages;
        std::vector<vk::ImageView> swapchainImageViews;
        utils::Image depthMap;
        utils::ImageView depthMapView;
        vk::Format depthMapFormat;
        vk::RenderPass renderPass;
        vk::SurfaceFormatKHR surfaceFormat;
        std::vector<vk::Framebuffer> framebuffers;
        std::vector<vk::CommandBuffer> frameCommandBuffers;
        std::map<std::tuple<uint32_t, uint32_t, uint32_t>, DrawComponents> drawComponents;
        std::vector<vk::CommandBuffer> presentCommandBuffers;
        std::tuple<uint32_t, uint32_t, uint32_t> currentPipeline;
        vk::Fence safeToRenderFence;
        vk::Fence safeToPresentFence;
        vk::Semaphore depthMapAvailableSemaphore;
        vk::Semaphore safeToRenderSemaphore;
        vk::Semaphore safeToPresentSemaphore;

        uint32_t width;
        uint32_t height;
        DrawOptions options;

        void createSwapchain();
        void createDepthMap();
        void createRenderPass();
        void createFramebuffers();
        std::pair<vk::VertexInputBindingDescription, std::vector<vk::VertexInputAttributeDescription> > createPipelineVertexInputStateBase(const BindingAlignmentInfo& vertexAlignmentInfo);
        void createPipeline(vk::Pipeline& pipeline, const std::vector<vk::PipelineShaderStageCreateInfo>& shaderStageInfos, const std::vector<BindingAlignmentInfo>& vertexAlignmentInfos, const vk::PipelineLayout& layout);
        void createCommandBuffers();
        void initCommandBuffers(DrawComponents& drawComponents, const std::vector<VertexBuffer*>& vertexBuffers);
    };

}

#endif