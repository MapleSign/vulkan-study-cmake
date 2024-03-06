#pragma once

#include "Rendering/VulkanSubpass.h"

#include "Rendering/VulkanGraphicsBuilder.h"

class LightingSubpass : public VulkanSubpass
{
public:
	LightingSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
		const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& renderPass, uint32_t subpass);
	~LightingSubpass();

	void prepare(const std::vector<const VulkanImageView*>& gBuffers);
	void update(float deltaTime, const Scene* scene) override;

	void draw(VulkanCommandBuffer& cmdBuf, const std::vector<VulkanDescriptorSet*>& globalSets) override;

private:
	SceneData defferedData;
	PushConstantRaster pushConstants{};
};