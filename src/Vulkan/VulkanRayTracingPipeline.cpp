#include "VulkanRayTracingPipeline.h"

#include <array>

VulkanRayTracingPipeline::VulkanRayTracingPipeline(const VulkanDevice& device) :
	device{ device }
{
	//// All stages
	//std::array<VkPipelineShaderStageCreateInfo, ShaderGroupCount> stages{};

	//VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	//stage.pName = "main";  // All the same entry point
	//// Raygen
	//stage.module = nvvk::createShaderModule(device.getHandle(), nvh::loadFile("spv/raytrace.rgen.spv", true, defaultSearchPaths, true));
	//stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	//stages[eRaygen] = stage;
	//// Miss
	//stage.module = nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytrace.rmiss.spv", true, defaultSearchPaths, true));
	//stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	//stages[eMiss] = stage;
	//// Hit Group - Closest Hit
	//stage.module = nvvk::createShaderModule(m_device, nvh::loadFile("spv/raytrace.rchit.spv", true, defaultSearchPaths, true));
	//stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	//stages[eClosestHit] = stage;
}

VulkanRayTracingPipeline::~VulkanRayTracingPipeline()
{
}
