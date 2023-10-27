#include "VulkanGraphicsBuilder.h"

VulkanGraphicsBuilder::VulkanGraphicsBuilder(
    const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent) :
    device{ device }, resManager{ resManager }, extent{ extent }
{
    shadowPass = std::make_unique<ShadowRenderPass>(device, resManager, VkExtent2D{ 4096, 4096 }, 1);

    VulkanImage offscreenColorImage{
            device, convert2Dto3D(extent),
            VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
    };
    device.getCommandPool().transitionImageLayout(offscreenColorImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, device.getGraphicsQueue());;
    renderTarget = VulkanRenderTarget::DEFAULT_CREATE_FUNC(std::move(offscreenColorImage));
    
    offscreenColor = &renderTarget->getViews()[0];
    offscreenDepth = &renderTarget->getViews()[1];

    auto attatchments = renderTarget->getAttatchments();
    attatchments.front().finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<LoadStoreInfo> loadStoreInfos{ attatchments.size() };
    renderPass = std::make_unique<VulkanRenderPass>(device, attatchments, loadStoreInfos);

    framebuffer = std::make_unique<VulkanFramebuffer>(device, *renderTarget, *renderPass);

    auto vertShader = resManager.createShaderModule("shaders/spv/shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    vertShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster), VK_SHADER_STAGE_GEOMETRY_BIT);

    vertShader.addShaderResourceUniform(ShaderResourceType::Uniform, 0, 0, 1, 
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 1);

    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 0, 1, VK_SHADER_STAGE_GEOMETRY_BIT);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 1);
    vertShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 2);

    auto fragShader = resManager.createShaderModule("shaders/spv/shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 2, 1, 
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 3, resManager.getTextureNum(), 
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 4, 1,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);

    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 0);
    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 1);
    fragShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 2);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 1, 3);

    renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader));
    renderPipeline->prepare();
    renderPipeline->getPipelineState();
    renderPipeline->recreatePipeline(extent, *renderPass);

    globalData = resManager.requireSceneData(*renderPipeline->getDescriptorSetLayouts()[0], 1,
        {
            {0, {device.getGPU().pad_uniform_buffer_size(sizeof(GlobalData)), 1}},
            {1, {device.getGPU().pad_uniform_buffer_size(sizeof(ObjectData) * resManager.getRenderMeshNum()), 1}},
            {2, {device.getGPU().pad_uniform_buffer_size(sizeof(ObjDesc) * resManager.getRenderMeshNum()), 1}}
        }
    );

    const auto& renderMeshes = resManager.getRenderMeshes();
    std::vector<ObjDesc> objDescs{ renderMeshes.size() };
    for (size_t i = 0; i < renderMeshes.size(); ++i) {
        auto& od = objDescs[i];
        auto& mesh = renderMeshes[i];
        od.vertexAddress = getBufferDeviceAddress(device.getHandle(), mesh.vertexBuffer.buffer) + mesh.vertexBuffer.offset;
        od.indexAddress = getBufferDeviceAddress(device.getHandle(), mesh.indexBuffer.buffer) + mesh.indexBuffer.offset;
        od.materialAddress = getBufferDeviceAddress(device.getHandle(), mesh.matBuffer.buffer) + mesh.matBuffer.offset;
        od.materialIndexAddress = getBufferDeviceAddress(device.getHandle(), mesh.matIndicesBuffer.buffer) + mesh.matIndicesBuffer.offset;
    }
    for (uint32_t i = 0; i < globalData.uniformBuffers.size(); ++i) {
        globalData.updateData(i, 2, objDescs.data(), sizeof(ObjDesc) * objDescs.size());
    }

    std::vector<VkDescriptorImageInfo> imageInfos{ resManager.getTextureNum() };
    std::transform(resManager.getTextures().begin(), resManager.getTextures().end(), imageInfos.begin(),
        [](const auto& tex) { return tex->getImageInfo(); });

    auto skyboxCubeMapImageInfo = resManager.getCubeMapTextures()[resManager.getSkybox().cubeMap]->getImageInfo();

    for (auto& dset : globalData.descriptorSets) {
        dset->addWriteArray(3, imageInfos);
        dset->addWrite(4, skyboxCubeMapImageInfo);
    }
    globalData.update();

    lightData = resManager.requireSceneData(*renderPipeline->getDescriptorSetLayouts()[1], 1,
        {
            {0, {device.getGPU().pad_uniform_buffer_size(sizeof(DirLight) * 16), 1}},
            {1, {device.getGPU().pad_uniform_buffer_size(sizeof(PointLight) * 16), 1}},
            {2, {device.getGPU().pad_uniform_buffer_size(sizeof(ShadowRenderPass::ShadowData)), 1}},
        }
    );

    VkSamplerCreateInfo shadowSamplerInfo{};
    auto shadowSampler = resManager.createSampler();
    for (auto& dset : lightData.descriptorSets) {
        dset->addWrite(3, 
            VkDescriptorImageInfo{
                shadowSampler,
                shadowPass->getShadowDepth()->getHandle(),
                VK_IMAGE_LAYOUT_GENERAL
            }
        );
    }
    lightData.update();

    skyboxPass = std::make_unique<SkyboxRenderPass>(device, resManager, extent, *renderPass);
}

VulkanGraphicsBuilder::~VulkanGraphicsBuilder()
{
    skyboxPass.reset();

    renderPipeline.reset();

    framebuffer.reset();
    renderPass.reset();
    renderTarget.reset();
}

void VulkanGraphicsBuilder::recreateGraphicsBuilder(const VkExtent2D extent)
{
    this->extent = extent;

    //change offscreenColor size
    VulkanImage offscreenColorImage{
        device, convert2Dto3D(extent),
        VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
    };
    device.getCommandPool().transitionImageLayout(offscreenColorImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, device.getGraphicsQueue());;
    renderTarget = VulkanRenderTarget::DEFAULT_CREATE_FUNC(std::move(offscreenColorImage));

    offscreenColor = &renderTarget->getViews()[0];
    offscreenDepth = &renderTarget->getViews()[1];

    auto attatchments = renderTarget->getAttatchments();
    attatchments.front().finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<LoadStoreInfo> loadStoreInfos{ attatchments.size() };
    renderPass = std::make_unique<VulkanRenderPass>(device, attatchments, loadStoreInfos);

    framebuffer = std::make_unique<VulkanFramebuffer>(device, *renderTarget, *renderPass);

    renderPipeline->recreatePipeline(extent, *renderPass);

    skyboxPass = std::make_unique<SkyboxRenderPass>(device, resManager, extent, *renderPass);
}

void VulkanGraphicsBuilder::update(float deltaTime, const Scene* scene)
{
    uint32_t currentImage = 0;
    const auto& camera = scene->getActiveCamera();

    GlobalData ubo{};
    ubo.view = camera->calcLookAt();
    ubo.proj = glm::perspective(glm::radians(camera->zoom), (float)extent.width / (float)extent.height, camera->zNear, camera->zFar);
    ubo.proj[1][1] *= -1;
    ubo.viewInverse = glm::inverse(ubo.view);
    ubo.projInverse = glm::inverse(ubo.proj);

    globalData.uniformBuffers[currentImage][0]->update(&ubo, sizeof(ubo));

    auto& buffer = globalData.uniformBuffers[currentImage][1];
    ObjectData* objData = reinterpret_cast<ObjectData*>(buffer->map());
    for (size_t i = 0; i < resManager.getRenderMeshNum(); ++i) {
        objData[i].model = resManager.getRenderMesh(i).tranformMatrix;
    }
    buffer->unmap();

    // Light Data
    std::vector<DirLight> dirLights;
    for (const auto& [name, light] : scene->getDirLightMap()) {
        dirLights.push_back(*light);
    }
    lightData.updateData(currentImage, 0, dirLights.data(), sizeof(DirLight) * dirLights.size());

    std::vector<PointLight> pointLights;
    for (const auto& [name, light] : scene->getPointLightMap()) {
        pointLights.push_back(*light);
    }
    lightData.updateData(currentImage, 1, pointLights.data(), sizeof(PointLight) * pointLights.size());

    //auto lightProj = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.1f, 60.f);
    //lightProj[1][1] *= -1;
    //auto lightView = glm::lookAt(dirLight->direction * -50.f, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    ShadowRenderPass::ShadowData shadowData{ /*lightProj * lightView*/ };
    lightData.updateData(currentImage, 2, &shadowData, sizeof(shadowData));

    pushConstants.dirLightNum = scene->getDirLightMap().size();
    pushConstants.pointLightNum = scene->getPointLightMap().size();
    pushConstants.viewPos = camera->position;

    shadowPass->update(deltaTime, scene);
    skyboxPass->update(deltaTime, scene);
}

void VulkanGraphicsBuilder::draw(VulkanCommandBuffer& cmdBuf, glm::vec4 clearColor)
{
    shadowPass->draw(cmdBuf, *globalData.descriptorSets[0], *lightData.descriptorSets[0]);

    std::vector<VkClearValue> clearValues{ 2 };
    //clearValues[0].color = {{0.2f, 0.3f, 0.3f, 1.0f}};
    clearValues[0].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    cmdBuf.beginRenderPass(*renderTarget, *renderPass, *framebuffer, clearValues, VK_SUBPASS_CONTENTS_INLINE);


    skyboxPass->draw(cmdBuf, *globalData.descriptorSets[0], *lightData.descriptorSets[0]);

    cmdBuf.bindPipeline(renderPipeline->getGraphicsPipeline());

    auto globalDescriptorSetHandle = globalData.descriptorSets[0]->getHandle();
    vkCmdBindDescriptorSets(cmdBuf.getHandle(),
        renderPipeline->getGraphicsPipeline().getBindPoint(),
        renderPipeline->getPipelineLayout().getHandle(),
        0, 1, &globalDescriptorSetHandle, 0, nullptr);

    auto lightDescriptorSetHandle = lightData.descriptorSets[0]->getHandle();
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

GraphicsRenderPass::GraphicsRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent) :
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

ShadowRenderPass::ShadowRenderPass(
    const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, uint32_t lightNum) :
    GraphicsRenderPass(device, resManager, extent)
{
    auto depthFormat = findDepthFormat(device.getGPU().getHandle());
    VulkanImage shadowDepthImage{
        device, convert2Dto3D(extent), depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        0,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1, lightNum
    };
    std::vector<VulkanImage> images{};
    images.push_back(std::move(shadowDepthImage));
    renderTarget = std::make_unique<VulkanRenderTarget>(std::move(images));

    shadowDepth = &renderTarget->getViews()[0];

    auto attatchments = renderTarget->getAttatchments();
    attatchments[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    std::vector<LoadStoreInfo> loadStoreInfos{ attatchments.size() };
    renderPass = std::make_unique<VulkanRenderPass>(device, attatchments, loadStoreInfos);

    framebuffer = std::make_unique<VulkanFramebuffer>(device, *renderTarget, *renderPass);

    auto vertShader = resManager.createShaderModule("shaders/spv/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    vertShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    vertShader.addShaderResourceUniform(ShaderResourceType::Uniform, 0, 0, 1,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 1);

    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 0);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 1);
    vertShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 2);

    std::unique_ptr<VulkanShaderModule> geomShader = 
        std::make_unique<VulkanShaderModule>(
            std::move(
                resManager.createShaderModule("shaders/spv/shadow.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT, "main")
            )
        );
    geomShader->addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    geomShader->addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 0);

    auto fragShader = resManager.createShaderModule("shaders/spv/shadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 2, 1,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 3, resManager.getTextureNum(),
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 4, 1,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);

    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 0);
    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 1);
    fragShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 2);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 1, 3);

    renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader), std::move(geomShader));
    renderPipeline->prepare();
    renderPipeline->recreatePipeline(extent, *renderPass);
}

void ShadowRenderPass::update(float deltaTime, const Scene* scene)
{
    pushConstants.dirLightNum = scene->getDirLightMap().size();
    pushConstants.pointLightNum = scene->getPointLightMap().size();
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

SkyboxRenderPass::SkyboxRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
    const VulkanRenderPass& graphicsRenderPass) :
    GraphicsRenderPass(device, resManager, extent), graphicsRenderPass{ graphicsRenderPass }
{
    auto vertShader = resManager.createShaderModule("shaders/spv/skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    vertShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster), VK_SHADER_STAGE_GEOMETRY_BIT);

    vertShader.addShaderResourceUniform(ShaderResourceType::Uniform, 0, 0, 1,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 1);

    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 0, 1, VK_SHADER_STAGE_GEOMETRY_BIT);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 1);
    vertShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 2);

    auto fragShader = resManager.createShaderModule("shaders/spv/skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 2, 1,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 3, resManager.getTextureNum(),
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 4, 1, 
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);

    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 0);
    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 1);
    fragShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 2);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 1, 3);

    renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader));
    renderPipeline->prepare();
    renderPipeline->getPipelineState().cullMode = VK_CULL_MODE_NONE;
    renderPipeline->getPipelineState().depthStencilState.depth_test_enable = VK_FALSE;
    renderPipeline->getPipelineState().depthStencilState.depth_write_enable = VK_FALSE;
    renderPipeline->recreatePipeline(extent, graphicsRenderPass);
}

void SkyboxRenderPass::draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet)
{
    const auto& skybox = resManager.getSkybox();

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

    vkCmdBindVertexBuffers(cmdBuf.getHandle(), 0, 1, &skybox.vertexBuffer.buffer, &skybox.vertexBuffer.offset);

    vkCmdBindIndexBuffer(cmdBuf.getHandle(), skybox.indexBuffer.buffer, skybox.indexBuffer.offset, skybox.indexType);


    cmdBuf.drawIndexed(skybox.indexNum, 1, 0, 0, 0);
}

void SkyboxRenderPass::update(float deltaTime, const Scene* scene)
{
}
