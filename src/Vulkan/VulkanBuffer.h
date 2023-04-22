#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"

class VulkanBuffer
{
public:
    VulkanBuffer(const VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer(const VulkanBuffer&) = delete;

    ~VulkanBuffer();

    void map(void*& target);
    void unmap();
    void update(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    VkDescriptorBufferInfo getBufferInfo() const;

    VkBuffer getHandle() const;

private:
	const VulkanDevice& device;

    VkDeviceSize offset{ 0 };
	VkDeviceSize size;
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags properties;

    VkBuffer buffer{ VK_NULL_HANDLE };
    VkDeviceMemory memory{ VK_NULL_HANDLE };

    void* mappedData{ nullptr };
};