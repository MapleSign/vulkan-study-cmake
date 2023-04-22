#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"

class VulkanSemaphore
{
public:
	VulkanSemaphore(const VulkanDevice& device);

	VulkanSemaphore(const VulkanSemaphore&) = delete;

	VulkanSemaphore(VulkanSemaphore&& other) noexcept;

	~VulkanSemaphore();

	VkSemaphore getHandle() const;

private:
	const VulkanDevice& device;
	VkSemaphore semaphore;
};
