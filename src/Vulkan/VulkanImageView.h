#pragma once

#include "VulkanCommon.h"
#include "VulkanImage.h"

class VulkanImageView
{
public:
    VulkanImageView(VulkanImage& image, VkFormat format = VK_FORMAT_UNDEFINED);

    VulkanImageView(VulkanImageView&) = delete;

    VulkanImageView(VulkanImageView&& other) noexcept;

    ~VulkanImageView();

    VkImageView getHandle() const;

    VulkanImage& getImage();
    constexpr const VulkanImage& getImage() const { return image; }

private:
    VkImageView imageView{};

    VulkanImage& image;
};