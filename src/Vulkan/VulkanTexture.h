#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "VulkanCommandPool.h"
#include "VulkanBuffer.h"
#include "VulkanQueue.h"

class VulkanTexture
{
public:
    VulkanTexture(
        const VulkanDevice& device, 
        const char* filename, 
        VkSampler sampler, 
        const VulkanCommandPool& commandPool, 
        const VulkanQueue& queue
    );

    VulkanTexture(
        const VulkanDevice& device, 
        const std::vector<std::string> filenames, 
        VkSampler sampler, 
        const VulkanCommandPool& commandPool, 
        const VulkanQueue& queue
    );

    ~VulkanTexture();

    VkDescriptorImageInfo getImageInfo() const;

private:
    const VulkanDevice& device;

    std::unique_ptr<VulkanImage> image;
    std::unique_ptr<VulkanImageView> imageView;
    VkSampler sampler;
    uint32_t mipLevels;
    uint32_t arrayLayers;
};