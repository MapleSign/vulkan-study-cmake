#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"

class VulkanDescriptorPool
{
public:
    VulkanDescriptorPool(const VulkanDevice& device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);

    ~VulkanDescriptorPool();

    VkDescriptorSet allocate(const VulkanDescriptorSetLayout& descSetLayout);

private:
	const VulkanDevice& device;
    VkDescriptorPool descriptorPool;

    uint32_t maxSets;
    std::vector<VkDescriptorPoolSize> poolSizes{};
};
