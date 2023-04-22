#include <unordered_map>

#include "VulkanRenderPipeline.h"

VulkanRenderPipeline::VulkanRenderPipeline(const VulkanDevice& device, VulkanShaderModule&& vertShader, VulkanShaderModule&& fragShader,
    const VkExtent2D extent, const VulkanRenderPass& renderPass) :
    device{ device }, vertShader{ std::move(vertShader) }, fragShader{ std::move(fragShader) }
{
    std::vector<VulkanShaderResource> shaderResources{};
    shaderResources.insert(shaderResources.end(), this->vertShader.getShaderResources().begin(), this->vertShader.getShaderResources().end());
    shaderResources.insert(shaderResources.end(), this->fragShader.getShaderResources().begin(), this->fragShader.getShaderResources().end());

    std::vector<VkPushConstantRange> pushConstantRanges;
    std::unordered_map<uint32_t, std::vector<VulkanShaderResource>> descriptorResourceSets;

    for (const auto& res : shaderResources) {
        if (res.type == ShaderResourceType::PushConstant) {
            auto it = std::find_if(pushConstantRanges.begin(), pushConstantRanges.end(), 
                [&](auto& pcRange) { return pcRange.offset == res.offset; });

            if (it != pushConstantRanges.end())
                it->stageFlags |= res.stageFlags;
            else
                pushConstantRanges.push_back({ res.stageFlags, res.offset, res.size });
        }
        else {
            if (descriptorResourceSets.end() != descriptorResourceSets.find(res.set)) {
                auto it = std::find_if(descriptorResourceSets[res.set].begin(), descriptorResourceSets[res.set].end(),
                    [&](auto& r) { return r.binding == res.binding; });

                if (it != descriptorResourceSets[res.set].end()) {
                    it->stageFlags |= res.stageFlags;
                    continue;
                }
            }

            descriptorResourceSets[res.set].push_back(res);
        }
    }

    for (const auto& [setIndex, setResources] : descriptorResourceSets) {
        descriptorSetLayouts.emplace_back(new VulkanDescriptorSetLayout(device, setIndex, setResources));
    }

    pipelineLayout = std::make_unique<VulkanPipelineLayout>(device, descriptorSetLayouts, pushConstantRanges);

    recreatePipeline(extent, renderPass);
}

VulkanRenderPipeline::~VulkanRenderPipeline()
{
}

void VulkanRenderPipeline::recreatePipeline(const VkExtent2D extent, const VulkanRenderPass& renderPass)
{
    std::vector<VkPipelineShaderStageCreateInfo> stageInfos{
        vertShader.getShaderStageInfo(),
        fragShader.getShaderStageInfo()
    };

    VulkanPipelineState pipelineState{
        pipelineLayout.get(),
        &renderPass, 0,
        extent,
        stageInfos,
        {Vertex::getBindingDescription()} ,
        Vertex::getAttributeDescriptions()
    };
    graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(device, pipelineState);
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
