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
    };

    class Texture
    {
        Texture(const uint32_t width, const uint32_t height, const void * rawData);
        const vk::ImageView& getImageView() const;
        vk::ImageView& getImageView();
        void bindMemory(const vk::CommandBuffer& memoryBindBuffer);
        ~Texture();
    private:
        ImageData imageData;
        AllocatedMemoryData memoryData;
        vk::Image image;
        vk::ImageView view;
        vk::Fence textureReadyFence;
        const void * rawImageData;

        void create();
    };

}

#endif