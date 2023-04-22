#pragma once

#include "VulkanCommon.h"
#include "VulkanCommandBuffer.h"
#include "VulkanQueue.h"
#include "VulkanImage.h"

class VulkanDevice;

class VulkanCommandPool
{
public:
    VulkanCommandPool(const VulkanDevice& device, uint32_t queueFamilyIndex);

    ~VulkanCommandPool();

    std::shared_ptr<VulkanCommandBuffer> beginSingleTimeCommands() const;

    void endSingleTimeCommands(VulkanCommandBuffer& commandBuffer, const VulkanQueue& queue) const;

    void transitionImageLayout(const VulkanImage& image, VkImageLayout oldLayout, VkImageLayout newLayout, const VulkanQueue& queue) const;

    void copyBufferToImage(const VulkanBuffer& buffer, const VulkanImage& image, const VulkanQueue& queue) const;

    void copyBuffer(VulkanBuffer& srcBuffer, VulkanBuffer& dstBuffer, VkDeviceSize size, const VulkanQueue& queue) const;

    void generateMipmaps(const VulkanImage& image, const VulkanQueue& queue) const;

    VkCommandPool getHandle() const;
    const VulkanDevice& getDevice() const;

private:
    const VulkanDevice& device;

    VkCommandPool commandPool;
};