#include "GlobalSubpass.h"

GlobalSubpass::GlobalSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
    const VulkanRenderPass& renderPass, uint32_t subpass) :
    VulkanSubpass(device, resManager, extent, {}, renderPass, subpass)
{
    auto vertShader = resManager.createShaderModule("shaders/spv/shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    vertShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    vertShader.addShaderResourceUniform(ShaderResourceType::Uniform, 0, 0, 1,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 1);

    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 0, 1, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 1, 1, 1, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR);
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
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 1, 3, 16, 0, ShaderResourceMode::UpdateAfterBind);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 1, 4, 16, 0, ShaderResourceMode::UpdateAfterBind);

    renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader));
    renderPipeline->prepare();
    auto& state = renderPipeline->getPipelineState();
    state.subpass = subpass;
    state.colorBlendAttachmentStates.resize(GBufferType::Count);
    renderPipeline->recreatePipeline(extent, renderPass);
}

GlobalSubpass::~GlobalSubpass()
{
}

void GlobalSubpass::prepare(
    const std::vector<std::unique_ptr<VulkanImageView>>& dirLightShadowMaps, 
    const std::vector<std::unique_ptr<VulkanImageView>>& pointLightShadowMaps)
{
    // create SceneData
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
            {2, {device.getGPU().pad_uniform_buffer_size(sizeof(ShadowData)), 1}},
        }
    );

    VkSamplerCreateInfo shadowSamplerInfo{};
    auto shadowSampler = resManager.createSampler();

    std::vector<VkDescriptorImageInfo> dirShadowImageInfos{};
    for (const auto& shadowDepth : dirLightShadowMaps) {
        dirShadowImageInfos.push_back({ shadowSampler, shadowDepth->getHandle(), VK_IMAGE_LAYOUT_GENERAL });
    }

    std::vector<VkDescriptorImageInfo> pointShadowImageInfos{};
    for (const auto& shadowDepth : pointLightShadowMaps) {
        pointShadowImageInfos.push_back({ shadowSampler, shadowDepth->getHandle(), VK_IMAGE_LAYOUT_GENERAL });
    }

    for (auto& dset : lightData.descriptorSets) {
        dset->addWriteArray(3, dirShadowImageInfos);
        dset->addWriteArray(4, pointShadowImageInfos);
    }
    lightData.update();
}

void GlobalSubpass::update(float deltaTime, const Scene* scene)
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

    pushConstants.dirLightNum = scene->getDirLightMap().size();
    pushConstants.pointLightNum = scene->getPointLightMap().size();
    pushConstants.viewPos = camera->position;
}

void GlobalSubpass::update(float deltaTime, const Scene* scene, ShadowData shadowData)
{
    uint32_t currentImage = 0;
    update(deltaTime, scene);
    lightData.updateData(currentImage, 2, &shadowData, sizeof(shadowData));
}

void GlobalSubpass::draw(VulkanCommandBuffer& cmdBuf, const std::vector<VulkanDescriptorSet>& globalSets)
{
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
}
