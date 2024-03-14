#include "VulkanGraphicsBuilder.h"

#include "Subpasses/GlobalSubpass.h"
#include "Subpasses/LightingSubpass.h"
#include "Subpasses/SkyboxSubpass.h"
#include "Subpasses/SSAOSubpass.h"

VulkanGraphicsBuilder::VulkanGraphicsBuilder(
    const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent) :
    device{ device }, resManager{ resManager }, extent{ extent }
{
    createRenderTarget();

    createRenderPass();

    framebuffer = std::make_unique<VulkanFramebuffer>(device, *renderTarget, *renderPass);

    globalPass = std::make_unique<GlobalSubpass>(device, resManager, extent, *renderPass, 1);

    std::vector<VulkanShaderResource> shaderResources = globalPass->getShaderResources();

    skyboxPass = std::make_unique<SkyboxSubpass>(device, resManager, extent, shaderResources, *renderPass, 0);
    ssaoPass = std::make_unique<SSAOSubpass>(device, resManager, extent, shaderResources, *renderPass, 2);
    ssaoBlurPass = std::make_unique<SSAOBlurSubpass>(device, resManager, extent, shaderResources, *renderPass, 3);
    lightingPass = std::make_unique<LightingSubpass>(device, resManager, extent, shaderResources, *renderPass, 4);

    dirShadowPass = std::make_unique<DirShadowRenderPass>(device, resManager, VkExtent2D{ 2048, 2048 }, shaderResources, shadowData.maxDirShadowNum, MAX_CSM_LEVEL);
    pointShadowPass = std::make_unique<PointShadowRenderPass>(device, resManager, VkExtent2D{ 2048, 2048 }, shaderResources, shadowData.maxPointShadowNum);

    globalPass->prepare(dirShadowPass->getShadowDepths(), pointShadowPass->getShadowDepths());
    ssaoPass->prepare(gBuffer);
    ssaoBlurPass->prepare(renderTarget->getViews());
    lightingPass->prepare(gBuffer);
    skyboxPass->prepare();
}

VulkanGraphicsBuilder::~VulkanGraphicsBuilder()
{
    dirShadowPass.reset();
    pointShadowPass.reset();

    skyboxPass.reset();
    ssaoPass.reset();
    ssaoBlurPass.reset();
    lightingPass.reset();

    globalPass.reset();

    framebuffer.reset();
    renderPass.reset();
    renderTarget.reset();
}

void VulkanGraphicsBuilder::recreateGraphicsBuilder(const VkExtent2D extent)
{
    this->extent = extent;

    createRenderTarget();

    createRenderPass();

    framebuffer = std::make_unique<VulkanFramebuffer>(device, *renderTarget, *renderPass);

    globalPass->recreatePipeline(extent, *renderPass);

    std::vector<VulkanShaderResource> shaderResources = globalPass->getShaderResources();
    skyboxPass = std::make_unique<SkyboxSubpass>(device, resManager, extent, shaderResources, *renderPass, 0);
    ssaoPass = std::make_unique<SSAOSubpass>(device, resManager, extent, shaderResources, *renderPass, 2);
    ssaoBlurPass = std::make_unique<SSAOBlurSubpass>(device, resManager, extent, shaderResources, *renderPass, 3);
    lightingPass = std::make_unique<LightingSubpass>(device, resManager, extent, shaderResources, *renderPass, 4);

    ssaoPass->prepare(gBuffer);
    ssaoBlurPass->prepare(renderTarget->getViews());
    lightingPass->prepare(gBuffer);
    skyboxPass->prepare();
}

void VulkanGraphicsBuilder::update(float deltaTime, const Scene* scene)
{
    globalPass->update(deltaTime, scene, shadowData);

    dirShadowPass->update(deltaTime, scene);
    pointShadowPass->update(deltaTime, scene);
    skyboxPass->update(deltaTime, scene);
    ssaoPass->update(deltaTime, scene);
    ssaoBlurPass->update(deltaTime, scene);
    lightingPass->update(deltaTime, scene);
}

void VulkanGraphicsBuilder::draw(VulkanCommandBuffer& cmdBuf, glm::vec4 clearColor)
{
    dirShadowPass->draw(cmdBuf, *(getGlobalData().descriptorSets[0]), *(getLightData().descriptorSets[0]));
    pointShadowPass->draw(cmdBuf, *(getGlobalData().descriptorSets[0]), *(getLightData().descriptorSets[0]));

    std::vector<VkClearValue> clearValues{ GBufferType::Total };
    //clearValues[0].color = {{0.2f, 0.3f, 0.3f, 1.0f}};
    clearValues[GBufferType::Color].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a} };
    clearValues[GBufferType::Depth].depthStencil = { 1.0f, 0 };
    cmdBuf.beginRenderPass(*renderTarget, *renderPass, *framebuffer, clearValues, VK_SUBPASS_CONTENTS_INLINE);

    std::vector<VulkanDescriptorSet*> globalSets = { getGlobalData().descriptorSets[0], getLightData().descriptorSets[0] };

    skyboxPass->draw(cmdBuf, globalSets);

    vkCmdNextSubpass(cmdBuf.getHandle(), VK_SUBPASS_CONTENTS_INLINE);
    
    globalPass->draw(cmdBuf, {});

    vkCmdNextSubpass(cmdBuf.getHandle(), VK_SUBPASS_CONTENTS_INLINE);

    ssaoPass->draw(cmdBuf, globalSets);

    vkCmdNextSubpass(cmdBuf.getHandle(), VK_SUBPASS_CONTENTS_INLINE);

    ssaoBlurPass->draw(cmdBuf, globalSets);

    vkCmdNextSubpass(cmdBuf.getHandle(), VK_SUBPASS_CONTENTS_INLINE);

    lightingPass->draw(cmdBuf, globalSets);

    cmdBuf.endRenderPass();
}

inline constexpr const SceneData& VulkanGraphicsBuilder::getGlobalData() const { return globalPass->getGlobalData(); }

inline constexpr const SceneData& VulkanGraphicsBuilder::getLightData() const { return globalPass->getLightData(); }

void VulkanGraphicsBuilder::createRenderTarget()
{
    VulkanImage offscreenColorImage{
               device, convert2Dto3D(extent),
               VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
    };
    // for RT to access when raster not running
    device.getCommandPool().transitionImageLayout(offscreenColorImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, device.getGraphicsQueue());;

    VkFormat depthFormat = findDepthFormat(device.getGPU().getHandle());
    VulkanImage offscreenDepthImage{
        device, convert2Dto3D(extent),
        depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    };

    VulkanImage gSceneColorImage{
        device, convert2Dto3D(extent),
        VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    };
    VulkanImage gPositionImage{
        device, convert2Dto3D(extent),
        VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    };
    VulkanImage gNormalImage{
        device, convert2Dto3D(extent),
        VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    };
    VulkanImage gAlbedoImage{
        device, convert2Dto3D(extent),
        VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    };
    VulkanImage gMetalRoughImage{
        device, convert2Dto3D(extent),
        VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    };

    VulkanImage ssaoImage{
        device, convert2Dto3D(extent),
        VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    };

    VulkanImage ssaoTmpImage{
        device, convert2Dto3D(extent),
        VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    };

    std::vector<VulkanImage> images{};
    images.push_back(std::move(gSceneColorImage));
    images.push_back(std::move(gPositionImage));
    images.push_back(std::move(gNormalImage));
    images.push_back(std::move(gAlbedoImage));
    images.push_back(std::move(gMetalRoughImage));
    images.push_back(std::move(ssaoImage));
    images.push_back(std::move(offscreenColorImage));
    images.push_back(std::move(offscreenDepthImage));
    images.push_back(std::move(ssaoTmpImage));

    renderTarget = std::make_unique<VulkanRenderTarget>(std::move(images));

    gBuffer.clear();
    for (size_t i = 0; i < GBufferType::Count; ++i) {
        gBuffer.push_back(&renderTarget->getViews()[i]);
    }
    offscreenColor = &renderTarget->getViews()[GBufferType::Color];
    offscreenDepth = &renderTarget->getViews()[GBufferType::Depth];
}

void VulkanGraphicsBuilder::createRenderPass()
{
    auto attatchments = renderTarget->getAttatchments();
    attatchments[GBufferType::Color].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<LoadStoreInfo> loadStoreInfos{ attatchments.size() };
    std::vector<SubpassInfo> subpassInfos = {
        SubpassInfo{ {GBufferType::Color} } , // Skybox
        SubpassInfo{ 
            {
                GBufferType::SceneColor, 
                GBufferType::Position, 
                GBufferType::Normal, 
                GBufferType::Albedo, 
                GBufferType::MetalRough, 
                GBufferType::Depth
            } 
        }, // Deffered
        SubpassInfo{ {GBufferType::Tmp}, {GBufferType::Position, GBufferType::Normal} }, // SSAO
        SubpassInfo{ {GBufferType::SSAO}, {GBufferType::Tmp} }, // SSAO Blur
        SubpassInfo{ 
            {GBufferType::Color}, 
            {
                GBufferType::SceneColor,
                GBufferType::Position,
                GBufferType::Normal,
                GBufferType::Albedo,
                GBufferType::MetalRough,
                GBufferType::SSAO
            }
        }, // Lighting
    };

    // Subpass dependencies
    {
        VkSubpassDependency dependency{};

        // ssao pass -> deffered pass
        dependency.srcSubpass = 1;
        dependency.dstSubpass = 2;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        subpassInfos[2].dependencies.push_back(dependency);

        // ssao blur pass -> ssao pass
        dependency.srcSubpass = 2;
        dependency.dstSubpass = 3;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        subpassInfos[3].dependencies.push_back(dependency);

        // lighting pass -> skybox pass
        dependency.srcSubpass = 0;
        dependency.dstSubpass = 4;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassInfos[4].dependencies.push_back(dependency);

        // lighting pass -> deffered pass
        dependency.srcSubpass = 1;
        dependency.dstSubpass = 4;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        subpassInfos[4].dependencies.push_back(dependency);

        // lighting pass -> ssao blur pass
        dependency.srcSubpass = 3;
        dependency.dstSubpass = 4;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        subpassInfos[4].dependencies.push_back(dependency);
    }

    renderPass = std::make_unique<VulkanRenderPass>(device, attatchments, loadStoreInfos, subpassInfos);
}

GraphicsRenderPass::GraphicsRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
    const std::vector<VulkanShaderResource> shaderRes) :
    device{ device }, resManager{ resManager }, extent{ extent }
{
}

GraphicsRenderPass::~GraphicsRenderPass()
{
    renderPipeline.reset();

    framebuffer.reset();
    renderPass.reset();
    renderTarget.reset();
}

ShadowRenderPass::ShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
    const std::vector<VulkanShaderResource> shaderRes, uint32_t maxLightNum) :
    GraphicsRenderPass(device, resManager, extent), maxLightNum{ maxLightNum }
{
}

void ShadowRenderPass::update(float deltaTime, const Scene* scene)
{
    pushConstants.dirLightNum = std::min(maxLightNum, toU32(scene->getDirLightMap().size()));
    pushConstants.pointLightNum = std::min(maxLightNum, toU32(scene->getPointLightMap().size()));
}

void ShadowRenderPass::draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet)
{
    std::vector<VkClearValue> clearValues{ 1 };
    //clearValues[0].color = {{0.2f, 0.3f, 0.3f, 1.0f}};
    clearValues[0].depthStencil = { 1.0f, 0 };
    cmdBuf.beginRenderPass(*renderTarget, *renderPass, *framebuffer, clearValues, VK_SUBPASS_CONTENTS_INLINE);


    cmdBuf.bindPipeline(renderPipeline->getGraphicsPipeline());

    auto globalDescriptorSetHandle = globalSet.getHandle();
    vkCmdBindDescriptorSets(cmdBuf.getHandle(),
        renderPipeline->getGraphicsPipeline().getBindPoint(),
        renderPipeline->getPipelineLayout().getHandle(),
        0, 1, &globalDescriptorSetHandle, 0, nullptr);

    auto lightDescriptorSetHandle = lightSet.getHandle();
    vkCmdBindDescriptorSets(cmdBuf.getHandle(),
        renderPipeline->getGraphicsPipeline().getBindPoint(),
        renderPipeline->getPipelineLayout().getHandle(),
        1, 1, &lightDescriptorSetHandle, 0, nullptr);

    for (size_t i = 0; i < resManager.getRenderMeshNum(); ++i) {
        const auto& renderMesh = resManager.getRenderMesh(i);
        pushConstants.objId = i;

        const auto& pipelineLayout = renderPipeline->getPipelineLayout();
        vkCmdPushConstants(cmdBuf.getHandle(), pipelineLayout.getHandle(),
            pipelineLayout.getPushConstantRanges()[0].stageFlags, 0, sizeof(PushConstantRaster), &pushConstants);

        vkCmdBindVertexBuffers(cmdBuf.getHandle(), 0, 1, &renderMesh.vertexBuffer.buffer, &renderMesh.vertexBuffer.offset);

        vkCmdBindIndexBuffer(cmdBuf.getHandle(), renderMesh.indexBuffer.buffer, renderMesh.indexBuffer.offset, renderMesh.indexType);


        cmdBuf.drawIndexed(renderMesh.indexNum, 1, 0, 0, 0);
    }


    cmdBuf.endRenderPass();
}

DirShadowRenderPass::DirShadowRenderPass(
    const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
    const std::vector<VulkanShaderResource> shaderRes, uint32_t maxLightNum, uint32_t maxCSMLevel) :
    ShadowRenderPass(device, resManager, extent, shaderRes, maxLightNum), maxCSMLevel{ maxCSMLevel }
{
    auto depthFormat = findDepthFormat(device.getGPU().getHandle());
    VulkanImage shadowDepthImage{
        device, convert2Dto3D(extent), depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        0,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1, maxLightNum * maxCSMLevel
    };
    std::vector<VulkanImage> images{};
    images.push_back(std::move(shadowDepthImage));
    renderTarget = std::make_unique<VulkanRenderTarget>(std::move(images));

    const auto& shadowImage = renderTarget->getImages()[0];
    for (uint32_t i = 0; i < maxLightNum; ++i) {
        shadowDepths.emplace_back(new VulkanImageView(shadowImage, VK_FORMAT_UNDEFINED, i, maxCSMLevel));
    }

    auto attatchments = renderTarget->getAttatchments();
    attatchments[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<LoadStoreInfo> loadStoreInfos{ attatchments.size() };
    renderPass = std::make_unique<VulkanRenderPass>(device, attatchments, loadStoreInfos);

    framebuffer = std::make_unique<VulkanFramebuffer>(device, *renderTarget, *renderPass);

    auto vertShader = resManager.createShaderModule("shaders/spv/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    vertShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));
    vertShader.addShaderResources(shaderRes);

    std::unique_ptr<VulkanShaderModule> geomShader =
        std::make_unique<VulkanShaderModule>(
            std::move(
                resManager.createShaderModule("shaders/spv/dirShadow.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT, "main")
            )
            );
    geomShader->addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    auto fragShader = resManager.createShaderModule("shaders/spv/dirShadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader), std::move(geomShader));
    renderPipeline->prepare();
    renderPipeline->recreatePipeline(extent, *renderPass);
}

PointShadowRenderPass::PointShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
    const std::vector<VulkanShaderResource> shaderRes, uint32_t maxLightNum) :
    ShadowRenderPass(device, resManager, extent, shaderRes, maxLightNum)
{
    auto depthFormat = findDepthFormat(device.getGPU().getHandle());
    VulkanImage shadowDepthImage{
        device, convert2Dto3D(extent), depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1, maxLightNum * 6
    };
    std::vector<VulkanImage> images{};
    images.push_back(std::move(shadowDepthImage));
    renderTarget = std::make_unique<VulkanRenderTarget>(std::move(images));

    const auto& shadowImage = renderTarget->getImages()[0];
    for (uint32_t i = 0; i < maxLightNum; ++i) {
        shadowDepths.emplace_back(new VulkanImageView(shadowImage, VK_FORMAT_UNDEFINED, i * 6, 6));
    }

    auto attatchments = renderTarget->getAttatchments();
    attatchments[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<LoadStoreInfo> loadStoreInfos{ attatchments.size() };
    renderPass = std::make_unique<VulkanRenderPass>(device, attatchments, loadStoreInfos);

    framebuffer = std::make_unique<VulkanFramebuffer>(device, *renderTarget, *renderPass);

    auto vertShader = resManager.createShaderModule("shaders/spv/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    vertShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));
    vertShader.addShaderResources(shaderRes);

    std::unique_ptr<VulkanShaderModule> geomShader =
        std::make_unique<VulkanShaderModule>(
            std::move(
                resManager.createShaderModule("shaders/spv/pointShadow.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT, "main")
            )
        );
    geomShader->addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    auto fragShader = resManager.createShaderModule("shaders/spv/pointShadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader), std::move(geomShader));
    renderPipeline->prepare();
    renderPipeline->recreatePipeline(extent, *renderPass);
}
