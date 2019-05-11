#ifndef SPARK_TEXTURE_HPP
#define SPARK_TEXTURE_HPP

#include"MemoryManager.hpp"
#include"System.hpp"
#include"Executives.hpp"

namespace spk
{

    struct ImageData
    {
        ImageData();
        operator vk::ImageCreateInfo();
        vk::ImageCreateFlags flags;
        vk::ImageType type;
        vk::Format format;
        vk::Extent3D extent;
        uint32_t mipLevels;
        uint32_t arrayLayers;
        vk::SampleCountFlagBits samples;
        vk::ImageTiling tiling;
        vk::ImageUsageFlags usage;
        vk::SharingMode sharingMode;
        uint32_t queueFamilyIndexCount;
        std::vector<uint32_t> queueFamilyIndices;
        vk::ImageLayout layout;
        uint32_t channelCount;
    };

    class Texture
    {
    public:
        Texture();
        Texture(const uint32_t width, const uint32_t height, const void * rawData, uint32_t cSetIndex, uint32_t cBinding);
        void create(const uint32_t width, const uint32_t height, const void * rawData, uint32_t cSetIndex, uint32_t cBinding);
        const vk::ImageView& getImageView() const;
        vk::ImageView& getImageView();
        void bindMemory(const vk::CommandBuffer& memoryBindBuffer);
        const vk::Fence& getReadyFence() const;
        Texture& operator=(const Texture& rTexture) = delete;
        Texture& operator=(Texture& rTexture) = delete;
        Texture& operator=(Texture&& rTexture);
        const uint32_t getSet() const;
        const uint32_t getBinding() const;
        ~Texture();
    private:
        ImageData imageData;
        AllocatedMemoryData memoryData;
        vk::Image image;
        vk::ImageView view;
        vk::Fence textureReadyFence;
        vk::Semaphore textureReadySemaphore;
        const void * rawImageData;
        uint32_t binding;
        uint32_t setIndex;
        bool transferred = false;

        void init();
    };

}

#endif