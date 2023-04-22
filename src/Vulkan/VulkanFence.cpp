#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanFence.h"

VulkanFence::VulkanFence(const VulkanDevice& device):
	device{device}
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	if (vkCreateFence(device.getHandle(), &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
		throw std::runtime_error("failed to create fence!");
	}
}

VulkanFence::VulkanFence(VulkanFence&& other) noexcept:
	device{other.device},
	fence{other.fence}
{
	other.fence = VK_NULL_HANDLE;
}

VulkanFence::~VulkanFence() {
	if (fence != VK_NULL_HANDLE)
		vkDestroyFence(device.getHandle(), fence, VK_NULL_HANDLE);
}

void VulkanFence::wait() {
	vkWaitForFences(device.getHandle(), 1, &fence, VK_TRUE, UINT64_MAX);
}

void VulkanFence::reset() {
	vkResetFences(device.getHandle(), 1, &fence);
}

VkFence VulkanFence::getHandle() const { return fence; }
