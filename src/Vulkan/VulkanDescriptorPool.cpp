#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"

VulkanDescriptorPool::VulkanDescriptorPool(
    const VulkanDevice& device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets) :
    device{ device }, poolSizes{ poolSizes }, maxSets{ maxSets }
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(this->poolSizes.size());
    poolInfo.pPoolSizes = this->poolSizes.data();
    poolInfo.maxSets = this->maxSets;

    if (vkCreateDescriptorPool(device.getHandle(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
    if (descriptorPool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device.getHandle(), descriptorPool, nullptr);
}

VkDescriptorSet VulkanDescriptorPool::allocate(const VulkanDescriptorSetLayout& descSetLayout) {
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descSetLayout.getHandle();

    if (vkAllocateDescriptorSets(device.getHandle(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    return descriptorSet;
}
