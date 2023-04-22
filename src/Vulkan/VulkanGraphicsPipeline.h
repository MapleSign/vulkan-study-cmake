#pragma once

#include "../Vertex.h"

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanShaderModule.h"
#include "VulkanPipelineLayout.h"
#include "VulkanRenderPass.h"
#include "VulkanPipeline.h"

struct VulkanPipelineState {
    const VulkanPipelineLayout* pipelineLayout;
    const VulkanRenderPass* renderPass;
    uint32_t subpass{ 0 };
    VkExtent2D extent;

    std::vector<VkPipelineShaderStageCreateInfo> stageInfos;

    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
};

class VulkanGraphicsPipeline : public VulkanPipeline
{
public:
    VulkanGraphicsPipeline(const VulkanDevice& device, const VulkanPipelineState& pipelineState);

    ~VulkanGraphicsPipeline();

private:
    const VulkanDevice& device;
    VulkanPipelineState state;
};