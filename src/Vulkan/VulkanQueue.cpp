#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanQueue.h"

VulkanQueue::VulkanQueue(const VulkanDevice& device, uint32_t familyIndex, VkQueueFamilyProperties properties):
	device{device}, familyIndex{familyIndex}, properties{properties}
{
	vkGetDeviceQueue(device.getHandle(), familyIndex, 0, &queue);
}

VulkanQueue::~VulkanQueue() {

}

void VulkanQueue::submit(const VulkanCommandBuffer& commandBuffer) const {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer.getHandle();

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void VulkanQueue::submit(const VulkanCommandBuffer& commandBuffer, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkPipelineStageFlags>& waitStages,
	const std::vector<VkSemaphore>& signalSemaphores, VkFence fence)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer.getHandle();

	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}

VkResult VulkanQueue::present(const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSwapchainKHR>& swapChains, uint32_t imageIndex) {
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	presentInfo.pWaitSemaphores = waitSemaphores.data();

	presentInfo.swapchainCount = static_cast<uint32_t>(swapChains.size());
	presentInfo.pSwapchains = swapChains.data();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	return vkQueuePresentKHR(queue, &presentInfo);
}

void VulkanQueue::waitIdle() const {
	vkQueueWaitIdle(queue);
}

VkQueue VulkanQueue::getHandle() const
{
	return queue;
}

uint32_t VulkanQueue::getFamilyIndex() const
{
	return familyIndex;
}
