#include"../include/ImageView.hpp"

namespace spk
{
    namespace utils
    {
        ImageView::ImageView(){}

        ImageView::ImageView(const vk::Image& image, const vk::Format format, const vk::ImageSubresourceRange range)
        {
            create(image, format, range);
        }

        void ImageView::create(const vk::Image& image, const vk::Format format, const vk::ImageSubresourceRange range)
        {
            const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
            
            vk::ImageViewCreateInfo viewInfo;
            viewInfo.setImage(image);
            viewInfo.setViewType(vk::ImageViewType::e2D);
            viewInfo.setFormat(format);
            viewInfo.setComponents(vk::ComponentMapping());
            viewInfo.setSubresourceRange(range);

            if(logicalDevice.createImageView(&viewInfo, nullptr, &view) != vk::Result::eSuccess) throw std::runtime_error("Failed to create image view!\n");
        }

        void ImageView::destroy()
        {
            if(view)
            {
                const vk::Device& logicalDevice = system::System::getInstance()->getLogicalDevice();
                logicalDevice.destroyImageView(view, nullptr);
                view = vk::ImageView();
            }
        }

        const vk::ImageView& ImageView::getView() const
        {
            return view;
        }

        ImageView::~ImageView()
        {
            destroy();
        }
    }
}