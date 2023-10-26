#include "VulkanCommon.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"

VulkanImageView::VulkanImageView(VulkanImage &image, VkFormat format):
    image{image}
{
    if (format == VK_FORMAT_UNDEFINED) {
        format = image.getFormat();
    }

    VkImageAspectFlags aspectFlags;
    if (isDepthStencilFormat(format)) {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else {
        aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageViewType viewType;
    if (image.getArrayLayers() > 1) {
        if (image.getFlags() & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
            viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        else viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
    else viewType = VK_IMAGE_VIEW_TYPE_2D;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image.getHandle();
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = image.getMipLevels();
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = image.getArrayLayers();

    if (vkCreateImageView(image.getDevice().getHandle(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}

VulkanImageView::VulkanImageView(VulkanImageView&& other) noexcept:
    image{other.image},
    imageView{other.imageView}
{
    other.imageView = VK_NULL_HANDLE;
}

VulkanImageView::~VulkanImageView() {
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(image.getDevice().getHandle(), imageView, nullptr);
    }
}

VkImageView VulkanImageView::getHandle() const { return imageView; }

VulkanImage& VulkanImageView::getImage() { return image; }
