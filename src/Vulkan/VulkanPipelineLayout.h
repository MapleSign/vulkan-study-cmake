#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"

class VulkanPipelineLayout
{
public:
	VulkanPipelineLayout(const VulkanDevice& device, 
		const std::vector<std::unique_ptr<VulkanDescriptorSetLayout>>& descriptorSetLayouts, 
		const std::vector<VkPushConstantRange>& pushConstantRanges);
	~VulkanPipelineLayout();

	VkPipelineLayout getHandle() const;

private:
	const VulkanDevice& device;
    VkPipelineLayout pipelineLayout;
};