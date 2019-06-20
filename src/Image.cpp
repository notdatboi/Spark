#include"../include/Image.hpp"

namespace spk
{
    namespace utils
    {
        const std::optional<vk::Format> Image::getSupportedFormat(const std::vector<vk::Format>& formats, const vk::ImageTiling tiling, const vk::FormatFeatureFlags flags)
        {
            const vk::PhysicalDevice& physicalDevice = system::System::getInstance()->getPhysicalDevice();
            std::optional<vk::Format> result;
            for(const auto fmt : formats)
            {
                vk::FormatProperties properties;
                physicalDevice.getFormatProperties(fmt, &properties);
                if(tiling == vk::ImageTiling::eLinear)
                {
                    if(properties.linearTilingFeatures & flags)
                    {
                        result = fmt;
                        break;
                    }
                }
                else
                {
                    if(properties.optimalTilingFeatures & flags)
                    {
                        result = fmt;
                        break;
                    }
                }
            }
            return result;
        }

        Image::Image(const Image& img)
        {
            destroy();
            create(img.extent, img.format, img.usage, img.subresourceRange.aspectMask);
        }

        Image& Image::operator=(const Image& img)
        {
            destroy();
            create(img.extent, img.format, img.usage, img.subresourceRange.aspectMask);
            return *this;
        }

        const vk::Image& Image::getImage() const
        {
            return image;
        }

        const vk::Format Image::getFormat() const
        {
            return format;
        }

        const vk::ImageSubresourceRange Image::getSubresource() const
        {
            return subresourceRange;
        }

        const vk::ImageLayout Image::getLayout() const
        {
            return layout;
        }

        Image::Image()
        {
            memoryData.index = ~0;
            memoryData.offset = ~0;
        }

        Image::Image(const vk::Extent3D cExtent, const vk::Format cFormat, const vk::ImageUsageFlags cUsage, const vk::ImageAspectFlags cAspectFlags)
        {
            memoryData.index = ~0;
            memoryData.offset = ~0;
            create(cExtent, cFormat, cUsage, cAspectFlags);
        }

        void Image::create(const vk::Extent3D cExtent, const vk::Format cFormat, const vk::ImageUsageFlags cUsage, const vk::ImageAspectFlags cAspectFlags)
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            extent = cExtent;
            layout = vk::ImageLayout::eUndefined;

            subresourceRange.setAspectMask(cAspectFlags);
            subresourceRange.setBaseMipLevel(0);
            subresourceRange.setLevelCount(1);
            subresourceRange.setBaseArrayLayer(0);
            subresourceRange.setLayerCount(1);

            std::vector<vk::Format> formats = {cFormat};

            vk::FormatFeatureFlags neededProperties;
            if(cUsage & vk::ImageUsageFlagBits::eColorAttachment) neededProperties |= vk::FormatFeatureFlagBits::eColorAttachment;
            if(cUsage & vk::ImageUsageFlagBits::eDepthStencilAttachment) neededProperties |= vk::FormatFeatureFlagBits::eDepthStencilAttachment;
            if(cUsage & vk::ImageUsageFlagBits::eTransferDst) neededProperties |= vk::FormatFeatureFlagBits::eTransferDst;

            vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
            std::optional formatAvailability = getSupportedFormat(formats, tiling, neededProperties);
            if(!formatAvailability.has_value())
            {
                tiling = vk::ImageTiling::eLinear;
                formatAvailability = getSupportedFormat(formats, tiling, neededProperties);
                if(!formatAvailability.has_value())
                {
                    throw std::invalid_argument("Unsupported format.\n");
                }
            }

            format = formatAvailability.value();
            usage = cUsage;
            vk::ImageCreateInfo info;
            info.setImageType(vk::ImageType::e2D);
            info.setFormat(format);
            info.setExtent(cExtent);
            info.setMipLevels(subresourceRange.levelCount);
            info.setArrayLayers(subresourceRange.layerCount);
            info.setSamples(vk::SampleCountFlagBits::e1);
            info.setTiling(tiling);
            info.setUsage(usage);
            info.setSharingMode(vk::SharingMode::eExclusive);
            info.setQueueFamilyIndexCount(1);
            const uint32_t graphicsFamilyIndex = system::Executives::getInstance()->getGraphicsQueueFamilyIndex();
            info.setPQueueFamilyIndices(&graphicsFamilyIndex);
            info.setInitialLayout(layout);

            if(logicalDevice.createImage(&info, nullptr, &image) != vk::Result::eSuccess)
            {
                throw std::runtime_error("Failed to create image!\n");
            }

            vk::MemoryRequirements memoryRequirements;
            logicalDevice.getImageMemoryRequirements(image, &memoryRequirements);

            system::MemoryAllocationInfo memoryInfo;
            memoryInfo.alignment = memoryRequirements.alignment;
            memoryInfo.size = memoryRequirements.size;
            memoryInfo.memoryTypeBits = memoryRequirements.memoryTypeBits;
            memoryInfo.flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
            memoryData = system::MemoryManager::getInstance()->allocateMemoryLazy(memoryInfo);
        }

        void Image::bindMemory()
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            const vk::DeviceMemory memory = system::MemoryManager::getInstance()->getMemory(memoryData.index);
            logicalDevice.bindImageMemory(image, memory, memoryData.offset);
        }

        void Image::changeLayout(vk::CommandBuffer& layoutChangeBuffer, 
            const vk::ImageLayout newLayout,
            const vk::Semaphore& waitSemaphore,
            const vk::Semaphore& signalSemaphore,
            const vk::Fence& waitFence,
            const vk::Fence& signalFence,
            bool oneTimeSubmit)
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            const vk::Queue& graphicsQueue = system::Executives::getInstance()->getGraphicsQueue();
            vk::PipelineStageFlags srcStage, dstStage;
            vk::AccessFlags srcAccess, dstAccess;
            if(layout == vk::ImageLayout::eUndefined)
            {
                srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
                if(newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
                {
                    dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
                    dstAccess = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                }
                else if(newLayout == vk::ImageLayout::eTransferDstOptimal)
                {
                    dstStage = vk::PipelineStageFlagBits::eTransfer;
                    dstAccess = vk::AccessFlagBits::eTransferWrite;
                }
                else if(newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
                {
                    dstStage = vk::PipelineStageFlagBits::eFragmentShader;
                    dstAccess = vk::AccessFlagBits::eShaderRead;
                }
                else throw std::invalid_argument("Unsupported layout transition.\n");
            }
            else if(layout == vk::ImageLayout::eTransferDstOptimal)
            {
                srcStage = vk::PipelineStageFlagBits::eTransfer;
                srcAccess = vk::AccessFlagBits::eTransferWrite;
                if(newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
                {
                    dstStage = vk::PipelineStageFlagBits::eFragmentShader;
                    dstAccess = vk::AccessFlagBits::eShaderRead;
                }
                else throw std::invalid_argument("Unsupported layout transition.\n");
            }
            else if(layout == vk::ImageLayout::eShaderReadOnlyOptimal)
            {
                srcStage = vk::PipelineStageFlagBits::eFragmentShader;
                srcAccess = vk::AccessFlagBits::eShaderRead;
                if(newLayout == vk::ImageLayout::eTransferDstOptimal)
                {
                    dstStage = vk::PipelineStageFlagBits::eTransfer;
                    dstAccess = vk::AccessFlagBits::eTransferWrite;
                }
                else throw std::invalid_argument("Unsupported layout transition.\n");
            }
            else throw std::invalid_argument("Unsupported layout transition.\n");


            vk::ImageMemoryBarrier barrier;
            barrier.setSrcAccessMask(srcAccess);
            barrier.setDstAccessMask(dstAccess);
            barrier.setOldLayout(layout);
            barrier.setNewLayout(newLayout);
            barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
            barrier.setImage(image);
            barrier.setSubresourceRange(subresourceRange);
            
            if(waitFence)
            {
                if(logicalDevice.waitForFences(1, &waitFence, true, ~0U) != vk::Result::eSuccess) throw std::runtime_error("Failed to wait for fences!\n");
                if(logicalDevice.resetFences(1, &waitFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset fence!\n");
            }

            vk::CommandBufferBeginInfo info;
            if(oneTimeSubmit)
            {
                info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
            }
            if(layoutChangeBuffer.begin(&info) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");
            layoutChangeBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
            layoutChangeBuffer.end();

            vk::SubmitInfo submit;
            if(waitSemaphore)
            {
                submit.setWaitSemaphoreCount(1);
                submit.setPWaitSemaphores(&waitSemaphore);
            }
            else
            {
                submit.setWaitSemaphoreCount(0);
                submit.setPWaitSemaphores(nullptr);
            }
            submit.setPWaitDstStageMask(&dstStage);
            submit.setCommandBufferCount(1);
            submit.setPCommandBuffers(&layoutChangeBuffer);
            if(signalSemaphore)
            {
                submit.setSignalSemaphoreCount(1);
                submit.setPSignalSemaphores(&signalSemaphore);
            }
            else
            {
                submit.setSignalSemaphoreCount(0);
                submit.setPSignalSemaphores(nullptr);
            }

            if(signalFence)
            {
                if(graphicsQueue.submit(1, &submit, signalFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to submit queue!\n");
            }
            else
            {
                if(graphicsQueue.submit(1, &submit, vk::Fence()) != vk::Result::eSuccess) throw std::runtime_error("Failed to submit queue!\n");
            }

            layout = newLayout;
        }

        void Image::update(vk::CommandBuffer& updateBuffer, 
            const vk::Buffer& buffer,
            const vk::Semaphore& waitSemaphore,
            const vk::Semaphore& signalSemaphore,
            const vk::Fence& waitFence,
            const vk::Fence& signalFence,
            const vk::PipelineStageFlags dstStageFlags,
            bool oneTimeSubmit)
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            const vk::Queue& graphicsQueue = system::Executives::getInstance()->getGraphicsQueue();
            if(layout != vk::ImageLayout::eTransferDstOptimal && layout != vk::ImageLayout::eGeneral) throw std::runtime_error("Can't update image with this layout!\n");

            vk::ImageSubresourceLayers subresource;
            subresource.setAspectMask(subresourceRange.aspectMask);
            subresource.setBaseArrayLayer(subresourceRange.baseArrayLayer);
            subresource.setLayerCount(subresourceRange.layerCount);
            subresource.setMipLevel(subresourceRange.baseMipLevel);

            vk::BufferImageCopy copyInfo;
            copyInfo.setBufferOffset(0);
            copyInfo.setBufferRowLength(0);
            copyInfo.setBufferImageHeight(0);
            copyInfo.setImageSubresource(subresource);
            copyInfo.setImageOffset(vk::Offset3D());
            copyInfo.setImageExtent(extent);

            if(waitFence)
            {
                if(logicalDevice.waitForFences(1, &waitFence, true, ~0U) != vk::Result::eSuccess) throw std::runtime_error("Failed to wait for fences!\n");
                if(logicalDevice.resetFences(1, &waitFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to reset fence!\n");
            }

            vk::CommandBufferBeginInfo info;
            if(oneTimeSubmit)
            {
                info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
            }

            if(updateBuffer.begin(&info) != vk::Result::eSuccess) throw std::runtime_error("Failed to begin command buffer!\n");
            updateBuffer.copyBufferToImage(buffer, image, layout, 1, &copyInfo);
            updateBuffer.end();

            vk::SubmitInfo submit;
            if(waitSemaphore)
            {
                submit.setWaitSemaphoreCount(1);
                submit.setPWaitSemaphores(&waitSemaphore);
            }
            else
            {
                submit.setWaitSemaphoreCount(0);
                submit.setPWaitSemaphores(nullptr);
            }
            submit.setPWaitDstStageMask(&dstStageFlags);
            submit.setCommandBufferCount(1);
            submit.setPCommandBuffers(&updateBuffer);
            if(signalSemaphore)
            {
                submit.setSignalSemaphoreCount(1);
                submit.setPSignalSemaphores(&signalSemaphore);
            }
            else
            {
                submit.setSignalSemaphoreCount(0);
                submit.setPSignalSemaphores(nullptr);
            }

            if(signalFence)
            {
                if(graphicsQueue.submit(1, &submit, signalFence) != vk::Result::eSuccess) throw std::runtime_error("Failed to submit queue!\n");
            }
            else
            {
                if(graphicsQueue.submit(1, &submit, vk::Fence()) != vk::Result::eSuccess) throw std::runtime_error("Failed to submit queue!\n");
            }
        }

        void Image::destroy()
        {
            if(image)
            {
                const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
                logicalDevice.destroyImage(image, nullptr);
                image = vk::Image();
            }
            if(memoryData.index != (~0) && memoryData.offset != (~0))
            {
                system::MemoryManager::getInstance()->freeMemory(memoryData.index);
                memoryData.index = ~0;
                memoryData.offset = ~0;
            }
        }

        Image::~Image()
        {
            destroy();
        }
    }
}