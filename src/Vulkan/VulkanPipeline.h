#pragma once

#include "VulkanCommon.h"

class VulkanPipeline
{
public:
	VulkanPipeline();
	~VulkanPipeline();

	virtual VkPipeline getHandle() const;
	virtual VkPipelineBindPoint getBindPoint() const;

protected:
	VkPipeline pipeline;
	VkPipelineBindPoint bindPoint;
};