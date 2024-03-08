#pragma once

#include "Rendering/VulkanSubpass.h"

#include "Rendering/VulkanGraphicsBuilder.h"

class SkyboxSubpass : public VulkanSubpass
{
public:
	SkyboxSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
		const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& renderPass, uint32_t subpass);
	~SkyboxSubpass();

	void update(float deltaTime, const Scene* scene) override;

	void draw(VulkanCommandBuffer& cmdBuf, const std::vector<VulkanDescriptorSet*>& globalSets) override;
};