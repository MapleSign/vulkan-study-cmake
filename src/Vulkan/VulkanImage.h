#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"

class VulkanImage
{
public:
    VulkanImage(const VulkanDevice& device, const VkExtent3D& extent, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mipLevels);

    VulkanImage(const VulkanDevice& device, VkImage handle, const VkExtent3D& extent, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels = 1);

    VulkanImage(VulkanImage&) = delete;

    VulkanImage(VulkanImage&& other) noexcept;

    ~VulkanImage();

    VkImage getHandle() const;
    VkDeviceMemory getMemory() const;

    const VkExtent3D& getExtent() const;
    VkFormat getFormat() const;
    VkSampleCountFlagBits getSampleCount() const;
    VkImageUsageFlags getUsage() const;
    uint32_t getMipLevels() const;

    const VulkanDevice& getDevice() const;

private:
    VkImage image{};
    VkDeviceMemory imageMemory{};

    VkExtent3D extent{};
    VkFormat format{};
    VkSampleCountFlagBits sampleCount{};
    VkImageUsageFlags usage{};
    uint32_t mipLevels{ 1 };

    const VulkanDevice &device;
};