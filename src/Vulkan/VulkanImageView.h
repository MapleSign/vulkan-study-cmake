#pragma once

#include "VulkanCommon.h"
#include "VulkanImage.h"

class VulkanImageView
{
public:
    VulkanImageView(const VulkanImage& image, VkFormat format = VK_FORMAT_UNDEFINED, uint32_t arrayLayer = 0, uint32_t layerCount = 0);

    VulkanImageView(VulkanImageView&) = delete;

    VulkanImageView(VulkanImageView&& other) noexcept;

    ~VulkanImageView();

    VkImageView getHandle() const;

    constexpr const VulkanImage& getImage() const { return image; }

private:
    VkImageView imageView{};

    const VulkanImage& image;
};