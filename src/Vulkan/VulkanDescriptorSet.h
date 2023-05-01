#pragma once

#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"

template<typename T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

class VulkanDescriptorSet
{
public:
	VulkanDescriptorSet(
		const VulkanDevice& device, 
		VulkanDescriptorPool& descPool,
		const VulkanDescriptorSetLayout& descSetLayout,
		const BindingMap<VkDescriptorBufferInfo>& bufferInfos = {},
		const BindingMap<VkDescriptorImageInfo>& imageInfos = {});

    ~VulkanDescriptorSet();

	void update();
	VkWriteDescriptorSet makeWrite(uint32_t	dstBinding, uint32_t arrayElement = 0);
	void addWrite(uint32_t dstBinding, const VkDescriptorBufferInfo& bufferInfo, uint32_t arrayElement = 0);
	void addWrite(uint32_t dstBinding, const VkDescriptorImageInfo& imageInfo, uint32_t arrayElement = 0);
	void addWrite(uint32_t dstBinding, const VkWriteDescriptorSetAccelerationStructureKHR* as, uint32_t arrayElement = 0);

	VkWriteDescriptorSet makeWriteArray(uint32_t dstBinding);
	void addWriteArray(uint32_t dstBinding, const VkDescriptorImageInfo* imageInfos);

    VkDescriptorSet getHandle() const;

private:
	const VulkanDevice& device;
	const VulkanDescriptorSetLayout& descSetLayout;
	VkDescriptorSet descriptorSet;

	BindingMap<VkDescriptorBufferInfo> bufferInfos;
	BindingMap<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet> descriptorWrites;
};
