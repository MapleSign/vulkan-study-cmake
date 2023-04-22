#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"

class VulkanCommandBuffer;

class VulkanQueue
{
public:
	VulkanQueue(const VulkanDevice& device, uint32_t familyIndex, VkQueueFamilyProperties properties = {});

	~VulkanQueue();

	void submit(const VulkanCommandBuffer& commandBuffer) const;

	void submit(const VulkanCommandBuffer& commandBuffer, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkPipelineStageFlags>& waitStages,
		const std::vector<VkSemaphore>& signalSemaphores, VkFence fence);

	VkResult present(const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSwapchainKHR>& swapChains, uint32_t imageIndex);

	void waitIdle() const;

	VkQueue getHandle() const;
	uint32_t getFamilyIndex() const;
private:
	const VulkanDevice& device;
	uint32_t familyIndex;
	VkQueueFamilyProperties properties;

	VkQueue queue;
};