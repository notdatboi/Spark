#ifndef SPARK_TEXTURE_HPP
#define SPARK_TEXTURE_HPP

#include"MemoryManager.hpp"
#include"System.hpp"
#include"Executives.hpp"
#include"Image.hpp"
#include"ImageView.hpp"

namespace spk
{
    enum class ImageFormat
    {
        RGBA8,
        BGRA8,
        RGBA16,
    };

    class Texture
    {
    public:
        Texture();
        Texture(const Texture& txt);
        Texture(const uint32_t cWidth, const uint32_t cHeight, ImageFormat cFormat, uint32_t cSetIndex, uint32_t cBinding);
        void create(const uint32_t cWidth, const uint32_t cHeight, ImageFormat cFormat, uint32_t cSetIndex, uint32_t cBinding);
        Texture& operator=(const Texture& rTexture);
        Texture& operator=(Texture& rTexture);
        void resetSetIndex(const uint32_t newIndex);
        void resetBinding(const uint32_t newBinding);
        ~Texture();
    private:
    
        struct ImageInfo
        {
            ImageInfo();
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

        friend class ResourceSet;
        const vk::ImageView& getImageView() const;
        const vk::ImageLayout getLayout() const;
        void bindMemory();
        void update(const void* rawData);
        const uint32_t getSet() const;
        const uint32_t getBinding() const;

        ImageInfo imageInfo;
        ImageFormat imageFormat;
        utils::Image image;
        utils::ImageView imageView;

        vk::CommandBuffer layoutChangeCB;
        vk::CommandBuffer imageUpdateCB;
        vk::Fence safeToCopyFence;
        vk::Semaphore safeToCopySemaphore;
        vk::Fence contentProcessedFence;
        vk::Semaphore contentProcessedSemaphore;
        vk::Fence safeToSampleFence;
        vk::Semaphore safeToSampleSemaphore;

        uint32_t binding;
        uint32_t setIndex;
        bool transferred = false;

        void init();
        void destroy();
    };
}

#endif