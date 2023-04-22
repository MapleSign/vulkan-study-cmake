#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanResource.h"
#include "VulkanRayTracingPipeline.h"

struct BuildAccelerationStructure
{
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo{ nullptr };

	VulkanAccelerationStructure as;
	VulkanAccelerationStructure cleanupAS;
};

class VulkanRayTracingBuilder
{
public:
	VulkanRayTracingBuilder(const VulkanDevice& device, VulkanResourceManager& resManager, const VulkanImageView& offscreenColor);
	~VulkanRayTracingBuilder();

	void createRayTracingPipeline(const std::vector<VulkanShaderModule>& rtShaders);

	void buildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags);

	void cmdCreateBlas(
		VkCommandBuffer cmdBuf,
		const std::vector<uint32_t>& indices,
		std::vector<BuildAccelerationStructure>& buildASs,
		VkDeviceAddress scratchAddress,
		VkQueryPool queryPool);

	void cmdCompactBlas(
		VkCommandBuffer cmdBuf,
		const std::vector<uint32_t>& indices,
		std::vector<BuildAccelerationStructure>& buildASs,
		VkQueryPool queryPool);

	void buildTlas(
		const std::vector<VkAccelerationStructureInstanceKHR>& instances, 
		VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, 
		bool update = false, bool motion = false);

	void cmdCreateTlas(
		VkCommandBuffer cmdBuf, 
		uint32_t countInstance, 
		VkDeviceAddress instBufferAddr, 
		VulkanBuffer*& scratchBuffer, 
		VkBuildAccelerationStructureFlagsKHR flags, 
		bool update, bool motion);

	VkDeviceAddress getBlasDeviceAddress(uint32_t blasID);
private:
	const VulkanDevice& device;
	VulkanResourceManager& resManager;
	const VulkanImageView* offscreenColor;

	std::vector<VulkanAccelerationStructure> blasList;
	VulkanAccelerationStructure tlas;

	std::unordered_map<uint32_t, std::unique_ptr<VulkanDescriptorSetLayout>> rtDescriptorSetLayouts;
	std::unordered_map<uint32_t, std::unique_ptr<VulkanDescriptorPool>> rtDescriptorPools;

	std::unique_ptr<VulkanDescriptorSet> rtDescriptorSet;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> rtShaderGroups;
};
