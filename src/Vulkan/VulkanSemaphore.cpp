#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanSemaphore.h"

VulkanSemaphore::VulkanSemaphore(const VulkanDevice& device):
	device{ device }, semaphore{}
{
	VkSemaphoreCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(device.getHandle(), &createInfo, VK_NULL_HANDLE, &semaphore);
}

VulkanSemaphore::VulkanSemaphore(VulkanSemaphore&& other) noexcept:
	device{ other.device }, semaphore{other.semaphore}
{
	other.semaphore = VK_NULL_HANDLE;
}

VulkanSemaphore::~VulkanSemaphore() {
	if (semaphore != VK_NULL_HANDLE)
		vkDestroySemaphore(device.getHandle(), semaphore, VK_NULL_HANDLE);
}

VkSemaphore VulkanSemaphore::getHandle() const { return semaphore; }
