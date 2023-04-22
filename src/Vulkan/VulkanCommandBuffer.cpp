#include <algorithm>

#include "VulkanCommon.h"
#include "VulkanCommandPool.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanFrameBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer(const VulkanCommandPool& commandPool, VkCommandBufferLevel level):
	commandPool{commandPool}
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool.getHandle();
	allocInfo.level = level;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(commandPool.getDevice().getHandle(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
	vkFreeCommandBuffers(commandPool.getDevice().getHandle(), commandPool.getHandle(), 1, &commandBuffer);
}

void VulkanCommandBuffer::begin(VkCommandBufferUsageFlags flags) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = flags;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

VkResult VulkanCommandBuffer::VulkanCommandBuffer::end() {
	return vkEndCommandBuffer(commandBuffer);
}

void VulkanCommandBuffer::reset(VkCommandBufferResetFlags flag)
{
	vkResetCommandBuffer(commandBuffer, flag);
}

void VulkanCommandBuffer::beginRenderPass(const VulkanRenderTarget& renderTarget, const VulkanRenderPass& renderPass, const VulkanFramebuffer& framebuffer, 
	const std::vector<VkClearValue>& clearValues, VkSubpassContents contents) 
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass.getHandle();
	renderPassInfo.framebuffer = framebuffer.getHandle();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = renderTarget.getExtent();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, contents);
}

void VulkanCommandBuffer::endRenderPass() {
	vkCmdEndRenderPass(commandBuffer);
}

void VulkanCommandBuffer::bindPipeline(const VulkanPipeline& pipeline) {
	vkCmdBindPipeline(commandBuffer, pipeline.getBindPoint(), pipeline.getHandle());
}

void VulkanCommandBuffer::bindVertexBuffers(uint32_t firstBinding, std::vector<std::reference_wrapper<VulkanBuffer>> buffers, std::vector<VkDeviceSize> offsets) {
	std::vector<VkBuffer> bufferHandles(buffers.size(), VK_NULL_HANDLE);
	std::transform(buffers.begin(), buffers.end(), bufferHandles.begin(),
		[](const VulkanBuffer& b) { return b.getHandle(); });
	vkCmdBindVertexBuffers(commandBuffer, firstBinding, static_cast<uint32_t>(bufferHandles.size()), bufferHandles.data(), offsets.data());
}

void VulkanCommandBuffer::bindIndexBuffer(const VulkanBuffer& buffer, VkDeviceSize offset, VkIndexType type) {
	vkCmdBindIndexBuffer(commandBuffer, buffer.getHandle(), offset, type);
}

void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
	vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::copyBufferToImage(const VulkanBuffer& buffer, const VulkanImage& image) {
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = image.getExtent();

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer.getHandle(),
		image.getHandle(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
}

void VulkanCommandBuffer::copyBuffer(VulkanBuffer& srcBuffer, VulkanBuffer& dstBuffer, VkDeviceSize size) {
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer.getHandle(), dstBuffer.getHandle(), 1, &copyRegion);
}

const VkCommandBuffer& VulkanCommandBuffer::getHandle() const { return commandBuffer; }
