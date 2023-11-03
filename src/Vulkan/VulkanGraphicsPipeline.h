#pragma once

#include "../Vertex.h"

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanShaderModule.h"
#include "VulkanPipelineLayout.h"
#include "VulkanPipeline.h"

class VulkanRenderPass;

struct DepthStencilState
{
    VkBool32 depth_test_enable{ VK_TRUE };
    VkBool32 depth_write_enable{ VK_TRUE };
};

struct ColorBlendAttachmentState
{
    VkColorComponentFlags colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
};

struct VulkanPipelineState {
    const VulkanPipelineLayout* pipelineLayout;
    const VulkanRenderPass* renderPass;
    uint32_t subpass{ 0 };
    VkExtent2D extent;

    std::vector<VkPipelineShaderStageCreateInfo> stageInfos;

    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;

    VkCullModeFlags cullMode{ VK_CULL_MODE_BACK_BIT };

    DepthStencilState depthStencilState{};
    std::vector<ColorBlendAttachmentState> colorBlendAttachmentStates{ 1 };
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