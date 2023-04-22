#pragma once

#include <map>

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanRenderPass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanShaderModule.h"

class VulkanRenderPipeline
{
public:
	VulkanRenderPipeline(const VulkanDevice& device, VulkanShaderModule&& vertShader, VulkanShaderModule&& fragShader, 
		const VkExtent2D extent, const VulkanRenderPass& renderPass);
	~VulkanRenderPipeline();

	void recreatePipeline(const VkExtent2D extent, const VulkanRenderPass& renderPass);

	const std::vector<std::unique_ptr<VulkanDescriptorSetLayout>>& getDescriptorSetLayouts() const;
	VulkanPipelineLayout& getPipelineLayout() const;
	VulkanGraphicsPipeline& getGraphicsPipeline() const;

private:
	const VulkanDevice& device;

	VulkanShaderModule vertShader;
	VulkanShaderModule fragShader;

	std::vector<std::unique_ptr<VulkanShaderModule>> shaderModules;

	std::vector<std::unique_ptr<VulkanDescriptorSetLayout>> descriptorSetLayouts;
	std::unique_ptr<VulkanPipelineLayout> pipelineLayout;
	std::unique_ptr<VulkanGraphicsPipeline> graphicsPipeline;
};
