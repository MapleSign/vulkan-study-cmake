#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanRenderTarget.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSemaphore.h"
#include "VulkanFence.h"
#include "VulkanFrameBuffer.h"
#include "VulkanRenderPass.h"

class VulkanRenderFrame
{
public:
	VulkanRenderFrame(VulkanDevice& device, std::unique_ptr<VulkanRenderTarget>&& renderTarget, const VulkanRenderPass& renderPass, VulkanCommandPool& commandPool);

	~VulkanRenderFrame();

	void wait();

	VulkanSemaphore& getImageAvailableSemaphore();

	VulkanSemaphore& getRenderFinishedSempahore();

	VulkanFence& getInFlightFence();

	VulkanCommandBuffer& getCommandBuffer();
	VulkanRenderTarget& getRenderTarget();
	VulkanFramebuffer& getFramebuffer();

private:
	VulkanDevice& device;
	std::unique_ptr<VulkanRenderTarget> renderTarget;

	VulkanSemaphore imageAvailableSemaphore;
	VulkanSemaphore renderFinishedSemaphore;
	VulkanFence inFlightFence;

	std::unique_ptr<VulkanFramebuffer> framebuffer;
	std::unique_ptr<VulkanCommandBuffer> commandBuffer;
};