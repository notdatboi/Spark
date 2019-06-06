#ifndef SPARK_IMAGE_VIEW_HPP
#define SPARK_IMAGE_VIEW_HPP

#include"SparkIncludeBase.hpp"
#include"System.hpp"

namespace spk
{
    namespace utils
    {
        class ImageView
        {
        public:
            ImageView();
            ImageView(const vk::Image& image, const vk::Format format, const vk::ImageSubresourceRange range);
            void create(const vk::Image& image, const vk::Format format, const vk::ImageSubresourceRange range);
            void destroy();
            const vk::ImageView& getView() const;
            ~ImageView();
        private:
            vk::ImageView view;
        };
    }
}

#endif