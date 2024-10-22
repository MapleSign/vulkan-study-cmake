#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanResource.h"
#include "VulkanPipelineLayout.h"
#include "VulkanRayTracingPipeline.h"

struct BuildAccelerationStructure
{
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo{ nullptr };

	VulkanAccelerationStructure as;
	VulkanAccelerationStructure cleanupAS;
};

struct PushConstantRayTracing {
	// 0
	glm::vec4 clearColor{};
	// 4
	glm::vec3 lightPosition{ -0.029, -1.00, -0.058 };
	float lightIntensity = 3.f;
	// 8
	int lightType = 1;
	int frame = -1;
	int maxDepth = 5;
	int sampleNumbers = 1;
	// 12
	// For Lens Approximation
	float defocusAngle = 0;
	float focusDist = 1;
	float zFar = 100;

	int dirLightNum;
	// 16
	int pointLightNum;
	glm::vec3 pad;
};

class VulkanRayTracingBuilder
{
public:
	VulkanRayTracingBuilder(const VulkanDevice& device, VulkanResourceManager& resManager, const VulkanImageView& offscreenColor);
	~VulkanRayTracingBuilder();

	void recreateRayTracingBuilder(const VulkanImageView& offscreenColor);

	void createRayTracingPipeline(
		const std::vector<VulkanShaderModule>& rtShaders, 
		const VulkanDescriptorSetLayout& globalDescSetLayout,
		const VulkanDescriptorSetLayout& lightDescSetLayout);
	void createRtShaderBindingTable();

	void raytrace(
		VulkanCommandBuffer& cmdBuf, 
		const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet, 
		const PushConstantRayTracing& pcRay);

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

	VkDeviceAddress getBlasDeviceAddress(uint32_t blasID) const;
private:
	const VulkanDevice& device;
	VulkanResourceManager& resManager;
	const VulkanImageView* offscreenColor;

	std::vector<VulkanAccelerationStructure> blasList;
	VulkanAccelerationStructure tlas;

	std::unordered_map<uint32_t, std::unique_ptr<VulkanDescriptorSetLayout>> rtDescriptorSetLayouts;

	VulkanDescriptorSet* rtDescriptorSet;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> rtShaderGroups;

	std::unique_ptr<VulkanPipelineLayout> rtPipelineLayout;
	std::unique_ptr<VulkanRayTracingPipeline> rtPipeline;

	VulkanBuffer* rtSBTBuffer;
	VkStridedDeviceAddressRegionKHR rgenRegion{};
	VkStridedDeviceAddressRegionKHR missRegion{};
	VkStridedDeviceAddressRegionKHR hitRegion{};
	VkStridedDeviceAddressRegionKHR callRegion{};
};
