#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"

class VulkanImage
{
public:
    VulkanImage(const VulkanDevice& device, const VkExtent3D& extent, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkImageCreateFlags flags = 0, VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        uint32_t mipLevels = 1, uint32_t arrayLayers = 1);

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
    uint32_t getArrayLayers() const;

    const VulkanDevice& getDevice() const;

private:
    VkImage image{};
    VkDeviceMemory imageMemory{};

    VkExtent3D extent{};
    VkFormat format{};
    VkSampleCountFlagBits sampleCount{};
    VkImageUsageFlags usage{};
    VkImageCreateFlags flags{};
    uint32_t mipLevels{ 1 };
    uint32_t arrayLayers{ 1 };
    VkImageLayout initialLayout{};

    const VulkanDevice &device;
};