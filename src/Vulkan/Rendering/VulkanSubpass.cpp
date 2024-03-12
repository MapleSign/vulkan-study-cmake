#include "VulkanSubpass.h"

VulkanSubpass::VulkanSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
	const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& renderPass, uint32_t subpass) :
	device{ device }, resManager{ resManager }, extent{ extent }, renderPass{ renderPass }, subpass{ subpass }
{
}

VulkanSubpass::~VulkanSubpass()
{
}
