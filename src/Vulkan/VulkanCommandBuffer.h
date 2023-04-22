#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanFrameBuffer.h"
#include "VulkanPipeline.h"

class VulkanCommandPool;

class VulkanCommandBuffer
{
public:
	VulkanCommandBuffer(const VulkanCommandPool& commandPool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	~VulkanCommandBuffer();

	void begin(VkCommandBufferUsageFlags flags);

	VkResult end();

	void reset(VkCommandBufferResetFlags flag);

	void beginRenderPass(const VulkanRenderTarget& renderTarget, const VulkanRenderPass& renderPass, const VulkanFramebuffer& framebuffer,
		const std::vector<VkClearValue>& clearValues, VkSubpassContents contents);

	void endRenderPass();

	void bindPipeline(const VulkanPipeline& pipeline);

	void bindVertexBuffers(uint32_t fistBinding, std::vector<std::reference_wrapper<VulkanBuffer>> buffers, std::vector<VkDeviceSize> offsets);

	void bindIndexBuffer(const VulkanBuffer& buffer, VkDeviceSize offset, VkIndexType type);

	void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

	void copyBufferToImage(const VulkanBuffer& buffer, const VulkanImage& image);

	void copyBuffer(VulkanBuffer& srcBuffer, VulkanBuffer& dstBuffer, VkDeviceSize size);

	const VkCommandBuffer& getHandle() const;

private:
	VkCommandBuffer commandBuffer;

	const VulkanCommandPool& commandPool;
};
