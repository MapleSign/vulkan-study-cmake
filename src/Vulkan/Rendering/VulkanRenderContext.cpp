#include <memory>
#include <vector>

#include "VulkanCommon.h"
#include "VulkanSwapChain.h"
#include "VulkanImageView.h"
#include "VulkanRenderPass.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanRenderFrame.h"
#include "VulkanCommandPool.h"
#include "VulkanRenderContext.h"

FrameSyncObject::FrameSyncObject(const VulkanDevice& device):
	imageAvailableSemaphores{device}, renderFinishedSemaphores{device}, inFlightFences{device}
{
}

VulkanRenderContext::VulkanRenderContext(VulkanDevice& device, VkSurfaceKHR surface, VkExtent2D extent, size_t threadCount):
	device{device}, surface{surface}, createFunc{VulkanRenderTarget::DEFAULT_CREATE_FUNC}, threadCount{threadCount}
{
	for (size_t i = 0; i < threadCount; ++i) {
		frameSyncObjects.emplace_back(device);
	}

	if (surface != VK_NULL_HANDLE)
		recreateSwapChain(extent);
}

VulkanRenderContext::~VulkanRenderContext() {
	swapChain.reset();
	renderPass.reset();
	frames.clear();
	frameSyncObjects.clear();
}

void VulkanRenderContext::generateFrames(VulkanRenderTarget::CreateFunc createFunc) {
	auto extent = swapChain->getExtent3D();
	for (auto imageHandle : swapChain->getImages()) {
		VulkanImage image{ device, imageHandle, extent, swapChain->getImageFormat(), swapChain->getImageUsage() };
		auto renderTarget = createFunc(std::move(image));
		frames.emplace_back(std::make_unique<VulkanRenderFrame>(device, std::move(renderTarget), *renderPass, device.getCommandPool()));
	}
}

VkResult VulkanRenderContext::beginFrame()
{
	auto& frameSyncObject = frameSyncObjects[syncIndex];
	frameSyncObject.inFlightFences.wait(); // if it wait, there is no frame spare, or it will do nothing.
	frameSyncObject.inFlightFences.reset();

	VkResult result;

	return result = swapChain->acquireNextImage(activeFrameIndex, frameSyncObject.imageAvailableSemaphores.getHandle(), VK_NULL_HANDLE);
}

VkResult VulkanRenderContext::submit(VulkanQueue& queue)
{
	auto& frameSyncObject = frameSyncObjects[syncIndex];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	std::vector<VkSemaphore> waitSemaphores = { frameSyncObject.imageAvailableSemaphores.getHandle() };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores = { frameSyncObject.renderFinishedSemaphores.getHandle() };

	queue.submit(getActiveFrame().getCommandBuffer(), waitSemaphores, waitStages, signalSemaphores, frameSyncObject.inFlightFences.getHandle());
	return VK_SUCCESS;
}

VkResult VulkanRenderContext::present(VulkanQueue& queue)
{
	auto& frameSyncObject = frameSyncObjects[syncIndex];
	std::vector<VkSemaphore> waitSemaphores = { frameSyncObject.renderFinishedSemaphores.getHandle() };

	std::vector<VkSwapchainKHR> swapChains = { swapChain->getHandle() };

	return queue.present(waitSemaphores, swapChains, activeFrameIndex);
}

void VulkanRenderContext::endFrame()
{
	syncIndex = (syncIndex + 1) % threadCount;
}

void VulkanRenderContext::drawFrame() {
	waitFrame();
	auto& prev_frame = getActiveFrame();

	uint32_t imageIndex;
	auto result = swapChain->acquireNextImage(imageIndex, prev_frame.getImageAvailableSemaphore().getHandle(), VK_NULL_HANDLE);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		handleSurfaceChange();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
}

void VulkanRenderContext::waitFrame() {
	getActiveFrame().wait();
}

void VulkanRenderContext::handleSurfaceChange() {

	VkSurfaceCapabilitiesKHR surface_properties{};
	
	while (surface_properties.currentExtent.width == 0xFFFFFFFF)
	{
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.getGPU().getHandle(),
			swapChain->getSurface(), &surface_properties));
	}

	device.waitIdle();

	recreateSwapChain(VkExtent2D{ surface_properties.currentExtent.width, surface_properties.currentExtent.height });
}

void VulkanRenderContext::recreateSwapChain(const VkExtent2D& extent) {
	swapChain.reset();
	swapChain = std::make_unique<VulkanSwapChain>(device, surface, extent);

	renderPass.reset();
	renderPass = std::make_unique<VulkanRenderPass>(device, swapChain->getImageFormat());

	frames.clear();
	generateFrames(createFunc);
}

VulkanRenderFrame& VulkanRenderContext::getActiveFrame() const {
	return *frames[activeFrameIndex];
}

uint32_t VulkanRenderContext::getActiveFrameIndex() const
{
	return activeFrameIndex;
}

uint32_t VulkanRenderContext::getSyncIndex() const
{
	return syncIndex;
}

VulkanSwapChain& VulkanRenderContext::getSwapChain() const { return *swapChain; }
VulkanRenderPass& VulkanRenderContext::getRenderPass() const {return *renderPass; }
const std::vector<std::unique_ptr<VulkanRenderFrame>>& VulkanRenderContext::getRenderFrames() const { return frames; }
