#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"

class VulkanFence
{
public:
	VulkanFence(const VulkanDevice& device);
	VulkanFence(VulkanFence&& other) noexcept;

	~VulkanFence();

	void wait();

	void reset();

	VkFence getHandle() const;

private:
	const VulkanDevice& device;
	VkFence fence;
};
