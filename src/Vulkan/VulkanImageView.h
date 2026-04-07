#pragma once

#include "VulkanCommon.h"
#include "VulkanImage.h"

class VulkanImageView
{
public:
    //@param image : image to view
    //@param format : VK_FORMAT_UNDEFINED for same format with image
    //@param arrayLayer : baseArrayLayer in subResourceRange
    //@param layerCount : layerCount in subResourceRange, 0 for same layerCount with image
    VulkanImageView(const VulkanImage& image, VkFormat format = VK_FORMAT_UNDEFINED, uint32_t baseLayer = 0, uint32_t layerCount = 0);

    VulkanImageView(VulkanImageView&) = delete;

    VulkanImageView(VulkanImageView&& other) noexcept;

    ~VulkanImageView();

    VkImageView getHandle() const { return imageView; }
    
    VkFormat getFormat() const { return format; }
    uint32_t getBaseLayer() const { return baseLayer; }
    uint32_t getLayerCount() const { return layerCount; }

    const VulkanImage& getImage() const { return image; }

private:
    VkImageView imageView{};
    
    VkFormat format;
    uint32_t baseLayer;
    uint32_t layerCount;

    const VulkanImage& image;
};
