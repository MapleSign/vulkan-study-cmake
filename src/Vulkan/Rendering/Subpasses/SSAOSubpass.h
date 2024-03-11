#pragma once

#include "Rendering/VulkanSubpass.h"

#include "Rendering/VulkanGraphicsBuilder.h"

struct SSAOData
{
	glm::mat4 view;
	glm::mat4 projection;

	glm::vec4 samples[64];

	glm::vec2 windowSize;
	int kernelSize = 64;
	float radius = 0.5;

	float bias = 0.025;
	glm::vec3 padding;
};

class SSAOSubpass : public VulkanSubpass
{
public:
	SSAOSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
		const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& renderPass, uint32_t subpass);
	~SSAOSubpass();

	void prepare(const std::vector<const VulkanImageView*>& gBuffers);
	void update(float deltaTime, const Scene* scene) override;

	void draw(VulkanCommandBuffer& cmdBuf, const std::vector<VulkanDescriptorSet*>& globalSets) override;

private:
	SSAOData ssaoData{};

	SceneData ssaoSceneData;
	std::unique_ptr<VulkanImage> noiseImage;
	std::unique_ptr<VulkanImageView> noiseImageView;
};