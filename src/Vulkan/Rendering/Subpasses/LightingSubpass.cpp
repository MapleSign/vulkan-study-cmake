#include "LightingSubpass.h"

LightingSubpass::LightingSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
	const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& renderPass, uint32_t subpass):
	VulkanSubpass(device, resManager, extent, shaderRes, renderPass, subpass)
{
    auto vertShader = resManager.createShaderModule("shaders/spv/passthrough.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    vertShader.addShaderResources(shaderRes);

    auto fragShader = resManager.createShaderModule("shaders/spv/lighting.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));
    for (int i = 0; i < GBufferType::Count; ++i) {
        fragShader.addShaderResourceUniform(ShaderResourceType::Input, 2, i);
    }

    renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader));
    renderPipeline->prepare();
    auto& state = renderPipeline->getPipelineState();
    state.subpass = subpass;
    state.cullMode = VK_CULL_MODE_NONE;
    state.depthStencilState.depth_test_enable = VK_FALSE;
    state.depthStencilState.depth_write_enable = VK_FALSE;
    state.vertexBindingDescriptions = {};
    state.vertexAttributeDescriptions = {};
    renderPipeline->recreatePipeline(extent, renderPass);
}

LightingSubpass::~LightingSubpass()
{
}

void LightingSubpass::prepare(const std::vector<const VulkanImageView*>& gBuffer)
{
    defferedData = resManager.requireSceneData(*renderPipeline->getDescriptorSetLayouts()[2], 1, {});
    for (int i = 0; i < GBufferType::Count; ++i) {
        defferedData.descriptorSets[0]->addWrite(i,
            VkDescriptorImageInfo{ VK_NULL_HANDLE, gBuffer[i]->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    }
    defferedData.update();
}

void LightingSubpass::update(float deltaTime, const Scene* scene)
{
    pushConstants.dirLightNum = scene->getDirLightMap().size();
    pushConstants.pointLightNum = scene->getPointLightMap().size();
    pushConstants.viewPos = scene->getActiveCamera()->position;
}

void LightingSubpass::draw(VulkanCommandBuffer& cmdBuf, const std::vector<VulkanDescriptorSet*>& globalSets)
{
    cmdBuf.bindPipeline(renderPipeline->getGraphicsPipeline());

    const auto& pipelineLayout = renderPipeline->getPipelineLayout();
    vkCmdPushConstants(cmdBuf.getHandle(), pipelineLayout.getHandle(),
        pipelineLayout.getPushConstantRanges()[0].stageFlags, 0, sizeof(PushConstantRaster), &pushConstants);

    std::vector<VkDescriptorSet> descSets = {
        globalSets[0]->getHandle(),
        globalSets[1]->getHandle(),
        defferedData.descriptorSets[0]->getHandle()
    };

    vkCmdBindDescriptorSets(cmdBuf.getHandle(),
        renderPipeline->getGraphicsPipeline().getBindPoint(),
        renderPipeline->getPipelineLayout().getHandle(),
        0, 3, descSets.data(), 0, nullptr);

    vkCmdDraw(cmdBuf.getHandle(), 3, 1, 0, 0);
}
