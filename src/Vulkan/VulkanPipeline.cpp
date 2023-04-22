#include "VulkanCommon.h"
#include "VulkanPipeline.h"

VulkanPipeline::VulkanPipeline() {}
VulkanPipeline::~VulkanPipeline() {}

VkPipeline VulkanPipeline::getHandle() const { return pipeline; }
VkPipelineBindPoint VulkanPipeline::getBindPoint() const { return bindPoint; }
