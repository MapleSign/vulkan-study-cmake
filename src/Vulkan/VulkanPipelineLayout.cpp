#include <vector>
#include <algorithm>

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanPipelineLayout.h"

VulkanPipelineLayout::VulkanPipelineLayout(const VulkanDevice& device,
    const std::vector<std::unique_ptr<VulkanDescriptorSetLayout>>& descriptorSetLayouts,
    const std::vector<VkPushConstantRange>& pushConstantRanges) :
    device{ device }, pushConstantRanges{ pushConstantRanges }
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandles{};
    descriptorSetLayoutHandles.reserve(descriptorSetLayouts.size());
    for (const auto& dsl : descriptorSetLayouts) {
        this->descriptorSetLayouts.push_back(dsl.get());
        descriptorSetLayoutHandles.push_back(dsl->getHandle());
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = toU32(descriptorSetLayoutHandles.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayoutHandles.data();
    pipelineLayoutInfo.pushConstantRangeCount = toU32(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data();

    if (vkCreatePipelineLayout(device.getHandle(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

VulkanPipelineLayout::~VulkanPipelineLayout() {
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device.getHandle(), pipelineLayout, nullptr);
    }
}

VkPipelineLayout VulkanPipelineLayout::getHandle() const { return pipelineLayout; }

const std::vector<VkPushConstantRange>& VulkanPipelineLayout::getPushConstantRanges() const
{
    return pushConstantRanges;
}
