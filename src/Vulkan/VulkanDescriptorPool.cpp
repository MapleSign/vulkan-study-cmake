#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"

VulkanDescriptorPool::VulkanDescriptorPool(const VulkanDevice& device, const VulkanDescriptorSetLayout& layout, uint32_t poolSize):
    device{device}, layout{layout}, poolSize{poolSize}
{
    const auto& bindings = layout.getBindings();
    poolSizes.resize(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        poolSizes[i].type = bindings[i].descriptorType;
        poolSizes[i].descriptorCount = bindings[i].descriptorCount * poolSize;
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(poolSize);

    if (vkCreateDescriptorPool(device.getHandle(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
    if (descriptorPool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device.getHandle(), descriptorPool, nullptr);
}

VkDescriptorSet VulkanDescriptorPool::allocate() {
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout.getHandle();

    if (vkAllocateDescriptorSets(device.getHandle(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    return descriptorSet;
}

uint32_t VulkanDescriptorPool::getSetIndex() const
{
    return layout.getSetIndex();
}

const VulkanDescriptorSetLayout& VulkanDescriptorPool::getLayout() const
{
    return layout;
}

VkDescriptorType VulkanDescriptorPool::getType(size_t i) const
{
    return layout.getBindings()[i].descriptorType;
}
