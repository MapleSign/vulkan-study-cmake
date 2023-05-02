#pragma once

#include "VulkanDevice.h"
#include "VulkanShaderModule.h"
#include "VulkanPipeline.h"
#include "VulkanPipelineLayout.h"

struct VulkanRTPipelineState {
    const VulkanPipelineLayout* pipelineLayout{ nullptr };

    std::vector<VkPipelineShaderStageCreateInfo> stageInfos;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> groupInfos;

    uint32_t maxPipelineRayRecursionDepth = 1;
};

class VulkanRayTracingPipeline : public VulkanPipeline
{
public:
    enum StageIndices
    {
        Raygen = 0,
        Miss,
        MissShadow,
        ClosestHit,
        
        ShaderGroupCount // always the last one
    };

	VulkanRayTracingPipeline(const VulkanDevice& device, const VulkanRTPipelineState& pipelineState);
	~VulkanRayTracingPipeline();

private:
    const VulkanDevice& device;

    VulkanRTPipelineState state;
};
