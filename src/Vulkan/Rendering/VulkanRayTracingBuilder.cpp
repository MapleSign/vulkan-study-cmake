#include "VulkanRayTracingBuilder.h"

VulkanRayTracingBuilder::VulkanRayTracingBuilder(
	const VulkanDevice& device, VulkanResourceManager& resManager, const VulkanImageView& offscreenColor) :
	device{ device }, resManager{ resManager }, offscreenColor{ &offscreenColor }
{
}

VulkanRayTracingBuilder::~VulkanRayTracingBuilder()
{
}

void VulkanRayTracingBuilder::createRayTracingPipeline(const std::vector<VulkanShaderModule>& rtShaders)
{
	std::vector<VulkanShaderResource> shaderResources{};
	for (const auto& shader : rtShaders) {
		shaderResources.insert(shaderResources.end(), shader.getShaderResources().begin(), shader.getShaderResources().end());
	}

	std::vector<VkPushConstantRange> pushConstantRanges;
	std::unordered_map<uint32_t, std::vector<VulkanShaderResource>> descriptorResourceSets;

	for (const auto& res : shaderResources) {
		if (res.type == ShaderResourceType::PushConstant) {
			pushConstantRanges.push_back({ res.stageFlags, res.offset, res.size });
			continue;
		}

		descriptorResourceSets[res.set].push_back(res);
	}

	for (const auto& [setIndex, setResources] : descriptorResourceSets) {
		rtDescriptorSetLayouts[setIndex] = std::make_unique<VulkanDescriptorSetLayout>(device, setIndex, setResources);
		rtDescriptorPools[setIndex] = std::make_unique<VulkanDescriptorPool>(device, *rtDescriptorSetLayouts[setIndex], 1);
	}
	
	VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
	descASInfo.accelerationStructureCount = 1;
	descASInfo.pAccelerationStructures = &tlas.handle;
	VkDescriptorImageInfo imageInfo{ {}, offscreenColor->getHandle(), VK_IMAGE_LAYOUT_GENERAL };

	rtDescriptorSet = std::make_unique<VulkanDescriptorSet>(device, *rtDescriptorPools[0]);
	rtDescriptorSet->addWrite(0, &descASInfo);
	rtDescriptorSet->addWrite(1, imageInfo);
	rtDescriptorSet->update();

	// Shader groups
	VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
	group.anyHitShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = VK_SHADER_UNUSED_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;
	group.intersectionShader = VK_SHADER_UNUSED_KHR;

	// Raygen
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = VulkanRayTracingPipeline::Raygen;
	rtShaderGroups.push_back(group);

	// Miss
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = VulkanRayTracingPipeline::Miss;
	rtShaderGroups.push_back(group);

	// closest hit shader
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = VulkanRayTracingPipeline::ClosestHit;
	rtShaderGroups.push_back(group);
}

void VulkanRayTracingBuilder::buildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags)
{
	uint32_t blasNum = toU32(input.size());
	VkDeviceSize blasSize{ 0 };
	uint32_t blasCompactNum{ 0 };
	VkDeviceSize maxScratchSize{ 0 };

	std::vector<BuildAccelerationStructure> buildASs{ blasNum };
	for (uint32_t i = 0; i < blasNum; ++i) {
		buildASs[i].buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildASs[i].buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildASs[i].buildInfo.flags = input[i].flags | flags;
		buildASs[i].buildInfo.geometryCount = toU32(input[i].asGeometry.size());
		buildASs[i].buildInfo.pGeometries = input[i].asGeometry.data();

		buildASs[i].rangeInfo = input[i].asBuildOffsetInfo.data();

		std::vector<uint32_t> maxPrimCount(input[i].asBuildOffsetInfo.size());
		for (auto j = 0; j < input[i].asBuildOffsetInfo.size(); j++)
			maxPrimCount[j] = input[i].asBuildOffsetInfo[j].primitiveCount;  // Number of primitives/triangles

		vkGetAccelerationStructureBuildSizesKHR(device.getHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&buildASs[i].buildInfo, maxPrimCount.data(), &buildASs[i].sizeInfo);
		
		blasSize += buildASs[i].sizeInfo.accelerationStructureSize;
		maxScratchSize = std::max(maxScratchSize, buildASs[i].sizeInfo.buildScratchSize);
		blasCompactNum += hasFlag(buildASs[i].buildInfo.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
	}

	auto& scratchBuffer = resManager.requireBuffer(
		maxScratchSize, 
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkDeviceAddress scratchAddress = getBufferDeviceAddress(device.getHandle(), scratchBuffer.getHandle());

	// Allocate a query pool for storing the needed size for every BLAS compaction.
	VkQueryPool queryPool{ VK_NULL_HANDLE };
	if (blasCompactNum > 0)  // Is compaction requested?
	{
		assert(blasCompactNum == blasNum);  // Don't allow mix of on/off compaction
		VkQueryPoolCreateInfo queryPoolCreateInfo{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		queryPoolCreateInfo.queryCount = blasNum;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
		vkCreateQueryPool(device.getHandle(), &queryPoolCreateInfo, nullptr, &queryPool);
	}

	// Batching creation/compaction of BLAS to allow staying in restricted amount of memory
	std::vector<uint32_t> indices;  // Indices of the BLAS to create
	VkDeviceSize batchSize{ 0 };
	VkDeviceSize batchLimit{ 256'000'000 };  // 256 MB
	auto& commandPool = device.getCommandPool();
	auto& queue = device.getGraphicsQueue();
	for (uint32_t i = 0; i < blasNum; i++)
	{
		indices.push_back(i);
		batchSize += buildASs[i].sizeInfo.accelerationStructureSize;
		// Over the limit or last BLAS element
		if (batchSize >= batchLimit || i == blasNum - 1)
		{
			auto commandBuffer = commandPool.beginSingleTimeCommands();
			cmdCreateBlas(commandBuffer->getHandle(), indices, buildASs, scratchAddress, queryPool);
			commandPool.endSingleTimeCommands(*commandBuffer, queue);

			if (queryPool)
			{
				auto commandBuffer = commandPool.beginSingleTimeCommands();
				cmdCompactBlas(commandBuffer->getHandle(), indices, buildASs, queryPool);
				commandPool.endSingleTimeCommands(*commandBuffer, queue);

				// Destroy the non-compacted version
				for (auto& i : indices) {
					vkDestroyAccelerationStructureKHR(device.getHandle(), buildASs[i].cleanupAS.handle, nullptr);
					resManager.destroyBuffer(buildASs[i].cleanupAS.buffer);
					buildASs[i].cleanupAS = {};
				}
			}
			// Reset

			batchSize = 0;
			indices.clear();
		}
	}

	for (auto& b : buildASs)
	{
		blasList.emplace_back(b.as);
	}

	// Clean up
	vkDestroyQueryPool(device.getHandle(), queryPool, nullptr);
	resManager.destroyBuffer(&scratchBuffer);
}

void VulkanRayTracingBuilder::cmdCreateBlas(
	VkCommandBuffer cmdBuf, 
	const std::vector<uint32_t>& indices, 
	std::vector<BuildAccelerationStructure>& buildASs, 
	VkDeviceAddress scratchAddress, 
	VkQueryPool queryPool)
{
	if (queryPool)
		vkResetQueryPool(device.getHandle(), queryPool, 0, toU32(indices.size()));

	uint32_t queryCount = 0;
	for (const auto& i : indices) {
		// Actual allocation of buffer and acceleration structure.
		VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		asCreateInfo.size = buildASs[i].sizeInfo.accelerationStructureSize; // Will be used to allocate memory.
		buildASs[i].as = resManager.requireAS(asCreateInfo);

		// BuildInfo #2 part
		buildASs[i].buildInfo.dstAccelerationStructure = buildASs[i].as.handle; // Setting where the build lands
		buildASs[i].buildInfo.scratchData.deviceAddress = scratchAddress; // All build are using the same scratch buffer

		// Building the bottom-level-acceleration-structure
		vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildASs[i].buildInfo, &buildASs[i].rangeInfo);

		// Since the scratch buffer is reused across builds, we need a barrier to ensure one build
		// is finished before starting the next one.
		VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

		if (queryPool)
		{
			// Add a query to find the 'real' amount of memory needed, use for compaction
			vkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuf, 1, &buildASs[i].buildInfo.dstAccelerationStructure,
				VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCount);
			queryCount += 1;
		}
	}
}

void VulkanRayTracingBuilder::cmdCompactBlas(
	VkCommandBuffer cmdBuf, 
	const std::vector<uint32_t>& indices, 
	std::vector<BuildAccelerationStructure>& buildASs, 
	VkQueryPool queryPool)
{
	uint32_t queryCount{ 0 };
	std::vector<VulkanAccelerationStructure> cleanupAS; // previous AS to destroy

	// Get the compacted size result back
	std::vector<VkDeviceSize> compactSizes(toU32(indices.size()));
	vkGetQueryPoolResults(
		device.getHandle(), queryPool, 
		0, toU32(compactSizes.size()), 
		compactSizes.size() * sizeof(VkDeviceSize), compactSizes.data(), sizeof(VkDeviceSize), 
		VK_QUERY_RESULT_WAIT_BIT);

	for (const auto& i : indices)
	{
		buildASs[i].cleanupAS = buildASs[i].as; // previous AS to destroy
		buildASs[i].sizeInfo.accelerationStructureSize = compactSizes[queryCount++];  // new reduced size

		// Creating a compact version of the AS
		VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		asCreateInfo.size = buildASs[i].sizeInfo.accelerationStructureSize;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildASs[i].as = resManager.requireAS(asCreateInfo);

		// Copy the original BLAS to a compact version
		VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
		copyInfo.src = buildASs[i].buildInfo.dstAccelerationStructure;
		copyInfo.dst = buildASs[i].as.handle;
		copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
		vkCmdCopyAccelerationStructureKHR(cmdBuf, &copyInfo);
	}
}

void VulkanRayTracingBuilder::buildTlas(
	const std::vector<VkAccelerationStructureInstanceKHR>& instances, 
	VkBuildAccelerationStructureFlagsKHR flags, 
	bool update, bool motion)
{
	// Cannot call buildTlas twice except to update.
	assert(tlas.handle == VK_NULL_HANDLE || update);

	uint32_t countInstance = toU32(instances.size());

	// Command buffer to create the TLAS
	auto& commandPool = device.getCommandPool();
	auto commandBuffer = commandPool.beginSingleTimeCommands(); //begin cmdbuffer

	// Create a buffer holding the actual instance data (matrices++) for use by the AS builder
	// Buffer of instances containing the matrices and BLAS ids
	auto& instancesBuffer = resManager.requireBufferWithData(
		instances, 
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkDeviceAddress instBufferAddr = getBufferDeviceAddress(device.getHandle(), instancesBuffer.getHandle());

	// Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
	VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	vkCmdPipelineBarrier(
		commandBuffer->getHandle(), 
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		0, 1, &barrier, 0, nullptr, 0, nullptr);

	// Creating the TLAS
	VulkanBuffer* scratchBuffer = nullptr;
	cmdCreateTlas(commandBuffer->getHandle(), countInstance, instBufferAddr, scratchBuffer, flags, update, motion);

	// Finalizing and destroying temporary data
	commandPool.endSingleTimeCommands(*commandBuffer, device.getGraphicsQueue());  // end cmdbuffer

	resManager.destroyBuffer(scratchBuffer);
	resManager.destroyBuffer(&instancesBuffer);
}

void VulkanRayTracingBuilder::cmdCreateTlas(
	VkCommandBuffer cmdBuf, 
	uint32_t countInstance, 
	VkDeviceAddress instBufferAddr, 
	VulkanBuffer*& scratchBuffer, 
	VkBuildAccelerationStructureFlagsKHR flags, 
	bool update, bool motion)
{
	// Wraps a device pointer to the above uploaded instances.
	VkAccelerationStructureGeometryInstancesDataKHR instancesVk{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
	instancesVk.data.deviceAddress = instBufferAddr;

	// Put the above into a VkAccelerationStructureGeometryKHR. We need to put the instances struct in a union and label it as instance data.
	VkAccelerationStructureGeometryKHR topASGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	topASGeometry.geometry.instances = instancesVk;

	// Find sizes
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	buildInfo.flags = flags;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &topASGeometry;
	buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	vkGetAccelerationStructureBuildSizesKHR(
		device.getHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
		&buildInfo, &countInstance, &sizeInfo);

	VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	createInfo.size = sizeInfo.accelerationStructureSize;

	tlas = resManager.requireAS(createInfo);

	// Allocate the scratch memory
	scratchBuffer = &resManager.requireBuffer(
		sizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkDeviceAddress scratchAddress = getBufferDeviceAddress(device.getHandle(), scratchBuffer->getHandle());

	// Update build information
	buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
	buildInfo.dstAccelerationStructure = tlas.handle;
	buildInfo.scratchData.deviceAddress = scratchAddress;

	// Build Offsets info: n instances
	VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo{ countInstance, 0, 0, 0 };
	const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

	// Build the TLAS
	vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfo, &pBuildOffsetInfo);
}

VkDeviceAddress VulkanRayTracingBuilder::getBlasDeviceAddress(uint32_t blasID)
{
	assert(size_t(blasID) < blasList.size());
	VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
	addressInfo.accelerationStructure = blasList[blasID].handle;
	return vkGetAccelerationStructureDeviceAddressKHR(device.getHandle(), &addressInfo);
}