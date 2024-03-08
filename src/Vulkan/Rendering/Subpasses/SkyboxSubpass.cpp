#include "SkyboxSubpass.h"

SkyboxSubpass::SkyboxSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
	const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& renderPass, uint32_t subpass):
	VulkanSubpass(device, resManager, extent, shaderRes, renderPass, subpass)
{
    auto vertShader = resManager.createShaderModule("shaders/spv/skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    vertShader.addShaderResources(shaderRes);

    auto fragShader = resManager.createShaderModule("shaders/spv/skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");

    renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader));
    renderPipeline->prepare();
    renderPipeline->getPipelineState().subpass = subpass;
    renderPipeline->getPipelineState().cullMode = VK_CULL_MODE_NONE;
    renderPipeline->getPipelineState().depthStencilState.depth_test_enable = VK_FALSE;
    renderPipeline->getPipelineState().depthStencilState.depth_write_enable = VK_FALSE;
    renderPipeline->recreatePipeline(extent, renderPass);
}

SkyboxSubpass::~SkyboxSubpass()
{
}

void SkyboxSubpass::update(float deltaTime, const Scene* scene)
{
}

void SkyboxSubpass::draw(VulkanCommandBuffer& cmdBuf, const std::vector<VulkanDescriptorSet*>& globalSets)
{
    const auto& skybox = resManager.getSkybox();

    cmdBuf.bindPipeline(renderPipeline->getGraphicsPipeline());

    auto globalDescriptorSetHandle = globalSets[0]->getHandle();
    vkCmdBindDescriptorSets(cmdBuf.getHandle(),
        renderPipeline->getGraphicsPipeline().getBindPoint(),
        renderPipeline->getPipelineLayout().getHandle(),
        0, 1, &globalDescriptorSetHandle, 0, nullptr);

    auto lightDescriptorSetHandle = globalSets[1]->getHandle();
    vkCmdBindDescriptorSets(cmdBuf.getHandle(),
        renderPipeline->getGraphicsPipeline().getBindPoint(),
        renderPipeline->getPipelineLayout().getHandle(),
        1, 1, &lightDescriptorSetHandle, 0, nullptr);

    vkCmdBindVertexBuffers(cmdBuf.getHandle(), 0, 1, &skybox.vertexBuffer.buffer, &skybox.vertexBuffer.offset);

    vkCmdBindIndexBuffer(cmdBuf.getHandle(), skybox.indexBuffer.buffer, skybox.indexBuffer.offset, skybox.indexType);


    cmdBuf.drawIndexed(skybox.indexNum, 1, 0, 0, 0);
}
