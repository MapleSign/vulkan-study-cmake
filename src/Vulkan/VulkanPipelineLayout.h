#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"

class VulkanPipelineLayout
{
public:
	VulkanPipelineLayout(const VulkanDevice& device, 
		const std::vector<VulkanDescriptorSetLayout*>& descriptorSetLayouts, 
		const std::vector<VkPushConstantRange>& pushConstantRanges);
	~VulkanPipelineLayout();

	VkPipelineLayout getHandle() const;
	const std::vector<VkPushConstantRange>& getPushConstantRanges() const;

private:
	const VulkanDevice& device;
	std::vector<VulkanDescriptorSetLayout*> descriptorSetLayouts;
	std::vector<VkPushConstantRange> pushConstantRanges;

    VkPipelineLayout pipelineLayout;
};