#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanRenderTarget.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSemaphore.h"
#include "VulkanFence.h"
#include "VulkanFrameBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderFrame.h"

VulkanRenderFrame::VulkanRenderFrame(VulkanDevice& device, std::unique_ptr<VulkanRenderTarget>&& renderTarget, const VulkanRenderPass& renderPass, VulkanCommandPool& commandPool) :
	device{ device }, renderTarget{ std::move(renderTarget) },
	imageAvailableSemaphore{ device }, renderFinishedSemaphore{ device }, inFlightFence{ device }
{
	framebuffer = std::make_unique<VulkanFramebuffer>(device, *(this->renderTarget), renderPass);
	commandBuffer = std::make_unique<VulkanCommandBuffer>(commandPool);
}

VulkanRenderFrame::~VulkanRenderFrame() {
	renderTarget.reset();
	framebuffer.reset();
	commandBuffer.reset();
}

void VulkanRenderFrame::wait() {
	inFlightFence.wait();
	inFlightFence.reset();
}

VulkanSemaphore& VulkanRenderFrame::getImageAvailableSemaphore() {
	return imageAvailableSemaphore;
}

VulkanSemaphore& VulkanRenderFrame::getRenderFinishedSempahore() {
	return renderFinishedSemaphore;
}

VulkanFence& VulkanRenderFrame::getInFlightFence() {
	return inFlightFence;
}

VulkanCommandBuffer& VulkanRenderFrame::getCommandBuffer() { return *commandBuffer; }
VulkanRenderTarget& VulkanRenderFrame::getRenderTarget() { return *renderTarget; }
VulkanFramebuffer& VulkanRenderFrame::getFramebuffer() { return *framebuffer; }
