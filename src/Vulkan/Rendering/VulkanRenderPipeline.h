#pragma once

#include <map>

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanRenderPass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanShaderModule.h"
#include "VulkanResource.h"

class VulkanRenderPipeline
{
public:
	VulkanRenderPipeline(const VulkanDevice& device, const VulkanResourceManager& resManager,
		VulkanShaderModule&& vertShader, VulkanShaderModule&& fragShader);
	~VulkanRenderPipeline();

	void recreatePipeline(const VkExtent2D extent, const VulkanRenderPass& renderPass);
	virtual void prepare();

	const std::vector<std::unique_ptr<VulkanDescriptorSetLayout>>& getDescriptorSetLayouts() const;
	VulkanPipelineLayout& getPipelineLayout() const;
	VulkanGraphicsPipeline& getGraphicsPipeline() const;

	constexpr VulkanPipelineState& getPipelineState() { return state; }

private:
	const VulkanDevice& device;
	const VulkanResourceManager& resManager;

	VulkanShaderModule vertShader;
	VulkanShaderModule fragShader;

	std::vector<std::unique_ptr<VulkanDescriptorSetLayout>> descriptorSetLayouts;
	std::unique_ptr<VulkanPipelineLayout> pipelineLayout;

	VulkanPipelineState state;
	std::unique_ptr<VulkanGraphicsPipeline> graphicsPipeline;
};
