#include "VulkanGraphicsBuilder.h"

VulkanGraphicsBuilder::VulkanGraphicsBuilder(
    const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent) :
    device{ device }, resManager{ resManager }, extent{ extent }
{
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
    vertShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));
    vertShader.addShaderResourceUniform(ShaderResourceType::Uniform, 0, 0, 1, 
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 1);

    auto fragShader = resManager.createShaderModule("shaders/spv/shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 2, 1, 
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 3, resManager.getTextureNum(), 
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR);

    fragShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 0);
    fragShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 1);

    renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader));
    renderPipeline->prepare();
    renderPipeline->recreatePipeline(extent, *renderPass);

    globalData = resManager.requireSceneData(*renderPipeline->getDescriptorSetLayouts()[0], 1,
        {
            {0, {device.getGPU().pad_uniform_buffer_size(sizeof(GlobalData)), 1}},
            {1, {device.getGPU().pad_uniform_buffer_size(sizeof(ObjectData) * resManager.getRenderMeshNum()), 1}},
        }
    );

    const auto& renderMeshes = resManager.getRenderMeshes();
    std::vector<ObjDesc> objDescs{ renderMeshes.size() };
    for (size_t i = 0; i < renderMeshes.size(); ++i) {
        auto& od = objDescs[i];
        auto& mesh = renderMeshes[i];
        od.vertexAddress = getBufferDeviceAddress(device.getHandle(), mesh.vertexBuffer.buffer);
        od.indexAddress = getBufferDeviceAddress(device.getHandle(), mesh.indexBuffer.buffer);
        od.materialAddress = getBufferDeviceAddress(device.getHandle(), mesh.matBuffer.buffer);
        od.materialIndexAddress = getBufferDeviceAddress(device.getHandle(), mesh.matIndicesBuffer.buffer);
    }
    auto& objDescBuffer = resManager.requireBufferWithData(objDescs, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::vector<VkDescriptorImageInfo> imageInfos{ resManager.getTextureNum() };
    std::transform(resManager.getTextures().begin(), resManager.getTextures().end(), imageInfos.begin(),
        [](const auto& tex) { return tex->getImageInfo(); });

    for (auto& dset : globalData.descriptorSets) {
        dset->addWrite(2, objDescBuffer.getBufferInfo());
        dset->addWriteArray(3, imageInfos.data());
    }
    globalData.update();

    lightData = resManager.requireSceneData(*renderPipeline->getDescriptorSetLayouts()[1], 1,
        {
            {0, {device.getGPU().pad_uniform_buffer_size(sizeof(DirLight)), 1}},
            {1, {device.getGPU().pad_uniform_buffer_size(sizeof(PointLight) * 16), 1}}
        }
    );
    lightData.update();
}

VulkanGraphicsBuilder::~VulkanGraphicsBuilder()
{
    renderPipeline.reset();

    framebuffer.reset();
    renderPass.reset();
    renderTarget.reset();
}

void VulkanGraphicsBuilder::update(float deltaTime, const Scene* scene)
{
    uint32_t currentImage = 0;
    const auto& camera = scene->getActiveCamera();

    GlobalData ubo{};
    ubo.view = camera->calcLookAt();
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 100.0f);
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

    const DirLight* dirLight = scene->getDirLight();
    lightData.uniformBuffers[currentImage][0]->update(dirLight, device.getGPU().pad_uniform_buffer_size(sizeof(DirLight)), 0);

    auto offset = device.getGPU().pad_uniform_buffer_size(sizeof(DirLight));
    std::vector<PointLight> pointLights;
    for (const auto& [name, light] : scene->getPointLightMap()) {
        pointLights.push_back(*light);
    }
    lightData.uniformBuffers[currentImage][0]->update(pointLights.data(), device.getGPU().pad_uniform_buffer_size(sizeof(PointLight) * pointLights.size()), offset);

    pushConstants.lightNum = scene->getPointLightMap().size();
    pushConstants.viewPos = camera->position;
}

void VulkanGraphicsBuilder::draw(VulkanCommandBuffer& cmdBuf, glm::vec4 clearColor)
{
    std::vector<VkClearValue> clearValues{ 2 };
    //clearValues[0].color = {{0.2f, 0.3f, 0.3f, 1.0f}};
    clearValues[0].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    cmdBuf.beginRenderPass(*renderTarget, *renderPass, *framebuffer, clearValues, VK_SUBPASS_CONTENTS_INLINE);


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
