#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"

class VulkanDescriptorPool
{
public:
    VulkanDescriptorPool(const VulkanDevice& device, const VulkanDescriptorSetLayout& layout, uint32_t poolSize);

    ~VulkanDescriptorPool();

    VkDescriptorSet allocate();

    uint32_t getSetIndex() const;

    const VulkanDescriptorSetLayout& getLayout() const;
    VkDescriptorType getType(size_t i) const;

private:
	const VulkanDevice& device;
	const VulkanDescriptorSetLayout& layout;
    VkDescriptorPool descriptorPool;

    uint32_t poolSize;
    std::vector<VkDescriptorPoolSize> poolSizes{};
};
