#pragma once

#include "VulkanDevice.h"
#include "VulkanShaderModule.h"
#include "VulkanPipeline.h"

class VulkanRayTracingPipeline : public VulkanPipeline
{
public:
    enum StageIndices
    {
        Raygen = 0,
        Miss,
        ClosestHit,
        
        ShaderGroupCount // always the last one
    };

	VulkanRayTracingPipeline(const VulkanDevice& device);
	~VulkanRayTracingPipeline();

private:
    const VulkanDevice& device;
};
