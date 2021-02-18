/**
 * @file ImageView.h
 * @brief Class containing image view
 * @version 0.1
 * @date 2021-02-12
 */
#pragma once
#include <vulkan/vulkan.h>

class ImageView {
public:
    ImageView();
    ImageView(VkImageView imageView);

    const VkImageView &getView() const { return _imageView; }
protected:
    VkImageView _imageView;
};
