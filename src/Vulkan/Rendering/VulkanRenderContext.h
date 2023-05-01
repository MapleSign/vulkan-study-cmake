#pragma once

#include <memory>
#include <vector>

#include "VulkanCommon.h"
#include "VulkanSwapChain.h"
#include "VulkanImageView.h"
#include "VulkanRenderPass.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanRenderFrame.h"
#include "VulkanCommandPool.h"

struct FrameSyncObject {
	FrameSyncObject(const VulkanDevice& device);

	VulkanSemaphore imageAvailableSemaphores;
	VulkanSemaphore renderFinishedSemaphores;
	VulkanFence inFlightFences;
};

class VulkanRenderContext
{
public:
	VulkanRenderContext(VulkanDevice& device, VkSurfaceKHR surface, VkExtent2D extent, 
		size_t threadCount = 1, VulkanRenderTarget::CreateFunc func = VulkanRenderTarget::DEFAULT_CREATE_FUNC);

	~VulkanRenderContext();

	void generateFrames(std::vector<std::unique_ptr<VulkanRenderTarget>>& renderTargets);

	VkResult beginFrame();
	VkResult submit(VulkanQueue& queue);
	VkResult present(VulkanQueue& queue);
	void endFrame();

	void drawFrame();

	void waitFrame();

	void handleSurfaceChange();

	void recreateSwapChain(const VkExtent2D& extent);

	void setCreateFunc(VulkanRenderTarget::CreateFunc func);

	VulkanRenderFrame& getActiveFrame() const;
	uint32_t getActiveFrameIndex() const;
	uint32_t getSyncIndex() const;

	VulkanSwapChain& getSwapChain() const;
	VulkanRenderPass& getRenderPass() const;
	const std::vector<std::unique_ptr<VulkanRenderFrame>>& getRenderFrames() const;

private:
	VulkanDevice& device;
	VkSurfaceKHR surface;
	std::unique_ptr<VulkanSwapChain> swapChain;
	std::unique_ptr<VulkanRenderPass> renderPass;

	VulkanRenderTarget::CreateFunc createFunc;

	uint32_t activeFrameIndex{ 0 };
	std::vector<std::unique_ptr<VulkanRenderFrame>> frames;

	size_t threadCount{ 1 };
	size_t syncIndex{ 0 };
	std::vector<FrameSyncObject> frameSyncObjects;
};