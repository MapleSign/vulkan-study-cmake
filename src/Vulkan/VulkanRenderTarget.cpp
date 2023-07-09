#include "VulkanRenderTarget.h"

const VulkanRenderTarget::CreateFunc VulkanRenderTarget::DEFAULT_CREATE_FUNC = [](VulkanImage&& image) -> std::unique_ptr<VulkanRenderTarget> {
    VkFormat depthFormat = findDepthFormat(image.getDevice().getGPU().getHandle());

    VulkanImage depthImage{ 
        image.getDevice(), image.getExtent(), 
        depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1
    };

    std::vector<VulkanImage> images;
    images.push_back(std::move(image));
    images.push_back(std::move(depthImage));

    return std::make_unique<VulkanRenderTarget>(std::move(images));
};

VulkanRenderTarget::VulkanRenderTarget(std::vector<VulkanImage>&& images):
    device{images.back().getDevice()}, images{std::move(images)}
{
    extent.height = this->images.front().getExtent().height;
    extent.width = this->images.front().getExtent().width;

    for (auto &image : this->images) {
        imageViews.emplace_back(image);
        attatchments.emplace_back(image.getFormat(), image.getSampleCount(), image.getUsage());
    }
}

VulkanRenderTarget::VulkanRenderTarget(std::vector<VulkanImageView>&& imageViews) :
    device{ imageViews.back().getImage().getDevice() }, images{}, imageViews{ std::move(imageViews) }
{
    extent.height = this->imageViews.front().getImage().getExtent().height;
    extent.width = this->imageViews.front().getImage().getExtent().width;

    for (auto &imageView : this->imageViews) {
        const auto& image = imageView.getImage();
        attatchments.emplace_back(image.getFormat(), image.getSampleCount(), image.getUsage());
    }
}

VulkanRenderTarget::~VulkanRenderTarget()
{
    imageViews.clear();
    images.clear();
}

const std::vector<VulkanImage>& VulkanRenderTarget::getImages() const { return images; }

const std::vector<VulkanImageView>& VulkanRenderTarget::getViews() const { return imageViews; }

VkExtent2D VulkanRenderTarget::getExtent() const { return extent; }

const std::vector<VulkanAttatchment>& VulkanRenderTarget::getAttatchments() const
{
    return attatchments;
}
