#include "VulkanRayTracingPipeline.h"

#include <array>

VulkanRayTracingPipeline::VulkanRayTracingPipeline(const VulkanDevice& device, const VulkanRTPipelineState& pipelineState) :
	VulkanPipeline{}, device{ device }, state{ pipelineState }
{
	bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;

	VkRayTracingPipelineCreateInfoKHR rtPipelineCreateInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
	rtPipelineCreateInfo.stageCount = toU32(pipelineState.stageInfos.size());
	rtPipelineCreateInfo.pStages = pipelineState.stageInfos.data();
	rtPipelineCreateInfo.groupCount = toU32(pipelineState.groupInfos.size());
	rtPipelineCreateInfo.pGroups = pipelineState.groupInfos.data();
	rtPipelineCreateInfo.layout = pipelineState.pipelineLayout->getHandle();
	rtPipelineCreateInfo.maxPipelineRayRecursionDepth = pipelineState.maxPipelineRayRecursionDepth;

	VK_CHECK(vkCreateRayTracingPipelinesKHR(device.getHandle(), {}, {}, 1, &rtPipelineCreateInfo, nullptr, &pipeline));
}

VulkanRayTracingPipeline::~VulkanRayTracingPipeline()
{
	vkDestroyPipeline(device.getHandle(), pipeline, nullptr);
}
