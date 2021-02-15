/**
 * @file TextureManager.cpp
 * @brief Create and control textures
 * @version 0.1
 * @date 2021-02-12
 */
#include "magma/vk/textures/TextureManager.h"

#include <vulkan/vulkan_core.h>

#include "magma/vk/logicalDevice.h"
#include "magma/vk/vulkan_common.h"

TextureManager::TextureManager(LogicalDevice &device) 
        : _device(device),
        _commandBuffers(device.handler(), device.getGraphicsCmdPool(), 1)
{}

TextureManager::~TextureManager() {
    _commandBuffers.freeCmdBuf(_device.handler(), _device.getGraphicsCmdPool());
}

const Texture &TextureManager::getTexture(const std::string &name) const { 
    return _textures.at(name); 
};

Texture &TextureManager::createTexture2D(const std::string &name, VkFormat format, VkExtent3D extent,
        VkImageUsageFlags usage, VkImageAspectFlags aspectMask) {
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = extent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.flags = 0;

    VkImage textureImage;
    VkResult result = vkCreateImage(_device.handler(), &imageInfo, nullptr, &textureImage);
    VK_CHECK_ERR(result, "TextureManager::failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device.handler(), textureImage, &memRequirements);
    VkDeviceMemory textureMemory = _device.createDeviceMemory(memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkBindImageMemory(_device.handler(), textureImage, textureMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView textureView;
    result = vkCreateImageView(_device.handler(), &viewInfo, nullptr, &textureView);
    VK_CHECK_ERR(result, "TextureManager::failed to create image view!");

    TextureInfo* textureInfo = new TextureInfo;
    textureInfo->imageInfo = imageInfo;
    textureInfo->viewInfo = viewInfo;
    textureInfo->curLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    _textures.emplace(name, Texture(textureImage, textureMemory, ImageView(textureView), textureInfo));
    return _textures.at(name);
}

static bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void TextureManager::setLayout(Texture &texture, VkImageLayout newLayout) {
    VkImageCreateInfo imageInfo = texture.getInfo()->imageInfo;
    _commandBuffers.resetCmdBuf(0);
    VkCommandBuffer cmdBuf = _commandBuffers.beginCmdBuf(0);
    {
        VkImageLayout oldLayout = texture.getInfo()->curLayout;
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.getImage();
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = imageInfo.mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = imageInfo.arrayLayers;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(imageInfo.format))
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask =
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
                cmdBuf,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
        );
    }
    _commandBuffers.endAndSubmitCmdBuf(0, _device.getGraphicsQueue());
    texture.getInfo()->curLayout = newLayout;
}

void TextureManager::copyFromBuffer(Texture &texture, VkBuffer buffer) {
    VkImageCreateInfo imageInfo = texture.getInfo()->imageInfo;
    _commandBuffers.resetCmdBuf(0);
    VkCommandBuffer cmdBuf = _commandBuffers.beginCmdBuf(0);
    {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = imageInfo.arrayLayers;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = imageInfo.extent;

        vkCmdCopyBufferToImage(
                cmdBuf,
                buffer, texture.getImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &region
        );
    }
    _commandBuffers.endAndSubmitCmdBuf(0, _device.getGraphicsQueue());
}

void TextureManager::deleteTexture(Texture &texture) {
    delete texture.getInfo();
    vkDestroyImageView(_device.handler(), texture.getView().getImageView(), nullptr);
    vkDestroyImage(_device.handler(), texture.getImage(), nullptr);
    vkFreeMemory(_device.handler(), texture.getMemory(), nullptr);
}