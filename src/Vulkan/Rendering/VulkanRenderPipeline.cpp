#include <unordered_map>

#include "VulkanRenderPipeline.h"

VulkanRenderPipeline::VulkanRenderPipeline(const VulkanDevice& device, const VulkanResourceManager& resManager,
    VulkanShaderModule&& vertShader, VulkanShaderModule&& fragShader) :
    device{ device }, resManager{ resManager }, vertShader{ std::move(vertShader) }, fragShader{ std::move(fragShader) }
{
}

VulkanRenderPipeline::~VulkanRenderPipeline()
{
    graphicsPipeline.reset();
    pipelineLayout.reset();
    descriptorSetLayouts.clear();
}

void VulkanRenderPipeline::prepare()
{
    std::vector<VulkanShaderResource> shaderResources{};
    shaderResources.insert(shaderResources.end(), this->vertShader.getShaderResources().begin(), this->vertShader.getShaderResources().end());
    shaderResources.insert(shaderResources.end(), this->fragShader.getShaderResources().begin(), this->fragShader.getShaderResources().end());

    std::vector<VkPushConstantRange> pushConstantRanges;
    std::unordered_map<uint32_t, std::vector<VulkanShaderResource>> descriptorResourceSets;
    
    createLayoutInfo(shaderResources, pushConstantRanges, descriptorResourceSets);

    for (const auto& [setIndex, setResources] : descriptorResourceSets) {
        descriptorSetLayouts.emplace_back(new VulkanDescriptorSetLayout(device, setIndex, setResources));
    }

    std::vector<VulkanDescriptorSetLayout*> descSetLayoutPointers{ descriptorSetLayouts.size() };
    std::transform(descriptorSetLayouts.begin(), descriptorSetLayouts.end(), descSetLayoutPointers.begin(),
        [](auto& dsl) { return dsl.get(); });
    pipelineLayout = std::make_unique<VulkanPipelineLayout>(device, descSetLayoutPointers, pushConstantRanges);

    std::vector<VkPipelineShaderStageCreateInfo> stageInfos{
        vertShader.getShaderStageInfo(),
        fragShader.getShaderStageInfo()
    };

    state.pipelineLayout = pipelineLayout.get();
    state.subpass = 0;
    state.stageInfos = stageInfos;
    state.vertexBindingDescriptions = { Vertex::getBindingDescription() };
    state.vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
}

void VulkanRenderPipeline::recreatePipeline(const VkExtent2D extent, const VulkanRenderPass& renderPass)
{
    state.extent = extent;
    state.renderPass = &renderPass;

    graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(device, state);
}

const std::vector<std::unique_ptr<VulkanDescriptorSetLayout>>& VulkanRenderPipeline::getDescriptorSetLayouts() const
{
    return descriptorSetLayouts;
}

VulkanPipelineLayout& VulkanRenderPipeline::getPipelineLayout() const
{
    return *pipelineLayout;
}

VulkanGraphicsPipeline& VulkanRenderPipeline::getGraphicsPipeline() const
{
    return *graphicsPipeline;
}
