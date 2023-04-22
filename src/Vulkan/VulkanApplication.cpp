#include <memory>
#include <vector>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Camera.h"
#include "../Platform/GlfwWindow.h"
#include "VulkanInclude.h"
#include "VulkanCommon.h"
#include "VulkanApplication.h"

VulkanApplication::VulkanApplication():
    threadCount{2}
{
    volkInitialize();

    window = std::make_unique<GlfwWindow>(*this);
    glfwSetCursorPosCallback(window->getHandle(), mouse_callback);

    instance = std::make_unique<VulkanInstance>(getRequiredExtensions(), validationLayers);

    surface = window->createSurface(instance->getHandle());

    device = std::make_unique<VulkanDevice>(instance->getSuitableGPU(surface, deviceExtensions), surface, deviceExtensions, validationLayers);

    resManager = std::make_unique<VulkanResourceManager>(*device, device->getCommandPool());

    scene = std::make_unique<Scene>();
    scene->addModel("nanosuit", "models/nanosuit/nanosuit.obj");
    scene->addPointLight("light0", { 0.f, 0.f, 10.f }, { 1.0f, 0.f, 0.f });
    scene->addPointLight("light1", { -40.f, 0.f, 10.f }, { 0.0f, 1.f, 0.f });

    VkSampler sampler = resManager->createSampler();
    for (const auto& [name, model] : scene->getModelMap()) {
        for (auto& mesh : model->getMeshes()) {
            std::vector<RenderTexture> textures;
            for (const auto& texture : mesh.textures) {
                textures.emplace_back(texture.path.c_str(), sampler);
            }

            renderMeshes.emplace(
                &mesh,
                resManager->requireRenderMesh(mesh.vertices, mesh.indices, mesh.mat, textures)
            );
        }
    }

    renderContext = std::make_unique<VulkanRenderContext>(*device, surface, window->getExtent(), threadCount);

    auto vertShader = resManager->createShaderModule("shaders/spv/vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    vertShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));
    vertShader.addShaderResourceUniform(ShaderResourceType::Uniform, 0, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    vertShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 1);

    auto fragShader = resManager->createShaderModule("shaders/spv/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(PushConstantRaster));

    fragShader.addShaderResourceUniform(ShaderResourceType::StorageBuffer, 0, 2, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 3, resManager->getTextureNum(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

    fragShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 0);
    fragShader.addShaderResourceUniform(ShaderResourceType::Uniform, 1, 1);

    renderPipeline = std::make_unique<VulkanRenderPipeline>(*device, std::move(vertShader), std::move(fragShader), 
        renderContext->getSwapChain().getExtent(), renderContext->getRenderPass());

    globalDescriptorPool = std::make_unique<VulkanDescriptorPool>(*device, *renderPipeline->getDescriptorSetLayouts()[0], threadCount);
    lightDescriptorPool = std::make_unique<VulkanDescriptorPool>(*device, *renderPipeline->getDescriptorSetLayouts()[1], threadCount);

    globalData = resManager->requireSceneData(*globalDescriptorPool, threadCount,
        {
            {0, {device->getGPU().pad_uniform_buffer_size(sizeof(GlobalData)), 1}}, 
            {1, {device->getGPU().pad_uniform_buffer_size(sizeof(ObjectData) * renderMeshes.size()), 1}},
        }
    );

    std::vector<ObjDesc> objDescs{ renderMeshes.size() };
    std::transform(renderMeshes.begin(), renderMeshes.end(), objDescs.begin(),
        [&](const auto& pair) { 
            ObjDesc od;
            auto& mesh = resManager->getRenderMesh(pair.second);
            od.vertexAddress = getBufferDeviceAddress(device->getHandle(), mesh.vertexBuffer.buffer);
            od.indexAddress = getBufferDeviceAddress(device->getHandle(), mesh.indexBuffer.buffer);
            od.materialAddress = getBufferDeviceAddress(device->getHandle(), mesh.matBuffer.buffer);
            od.materialIndexAddress = getBufferDeviceAddress(device->getHandle(), mesh.matIndicesBuffer.buffer);
            return od;
        });
    auto& objDescBuffer = resManager->requireBufferWithData(objDescs, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::vector<VkDescriptorImageInfo> imageInfos{ resManager->getTextureNum() };
    std::transform(resManager->getTextures().begin(), resManager->getTextures().end(), imageInfos.begin(), 
        [](const auto& tex) { return tex->getImageInfo(); });

    for (auto& dset : globalData.descriptorSets) {
        dset->addWrite(2, objDescBuffer.getBufferInfo());
        dset->addWriteArray(3, imageInfos.data());
    }
    globalData.update();

    lightData = resManager->requireSceneData(*lightDescriptorPool, threadCount,
        {
            {0, {device->getGPU().pad_uniform_buffer_size(sizeof(DirLight)), 1}},
            {1, {device->getGPU().pad_uniform_buffer_size(sizeof(PointLight) * 16), 1}}
        }
    ); 
    lightData.update();

    //rtBuilder = std::make_unique<VulkanRayTracingBuilder>(device);

    //// BLAS - Storing each primitive in a geometry
    //std::vector<BlasInput> allBlas;
    //allBlas.reserve(renderMeshes.size());
    //for (const auto& [p_mesh, id] : renderMeshes)
    //{
    //    auto blas = resManager->requireBlasInput(resManager->getRenderMesh(id));

    //    // We could add more geometry in each BLAS, but we add only one for now
    //    allBlas.emplace_back(blas);
    //}
    //rtBuilder->buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

    //// TLAS
    //std::vector<VkAccelerationStructureInstanceKHR> tlas;
    //tlas.reserve(renderMeshes.size());
    //uint32_t i = 0;
    //for (const auto& [p_mesh, id] : renderMeshes)
    //{
    //    VkAccelerationStructureInstanceKHR rayInst{};
    //    rayInst.transform = toTransformMatrixKHR(resManager->getRenderMesh(id).tranformMatrix); // Position of the instance
    //    rayInst.instanceCustomIndex = 0; // gl_InstanceCustomIndexEXT
    //    rayInst.accelerationStructureReference = rtBuilder->getBlasDeviceAddress(i);
    //    rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    //    rayInst.mask = 0xFF; //  Only be hit if rayMask & instance.mask != 0
    //    rayInst.instanceShaderBindingTableRecordOffset = 0; // We will use the same hit group for all objects
    //    tlas.emplace_back(rayInst);

    //    ++i;
    //}
    //rtBuilder->buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

    //std::vector<VulkanShaderModule> rtShaders{};
    //rtShaders.emplace_back(resManager->createShaderModule("shaders/raytrace.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR, "main"));
    //rtShaders.back().addShaderResourceUniform(ShaderResourceType::AccelerationStructure, 0, 0);
    //rtShaders.back().addShaderResourceUniform(ShaderResourceType::StorageImage, 0, 1);
    //rtShaders.emplace_back(resManager->createShaderModule("shaders/raytrace.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR, "main"));
    //rtShaders.emplace_back(resManager->createShaderModule("shaders/raytrace.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "main"));



    gui = std::make_unique<GUI>(*instance, *window, *device, renderContext->getRenderPass());
}

VulkanApplication::~VulkanApplication()
{
    resManager.reset();
    globalDescriptorPool.reset();
    lightDescriptorPool.reset();

    gui.reset();

    renderPipeline.reset();
    renderContext.reset();

    device.reset();

    vkDestroySurfaceKHR(instance->getHandle(), surface, nullptr);
    instance.reset();
    window.reset();
}

void VulkanApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window->getHandle()))
    {
        glfwPollEvents();

        gui->newFrame();

        drawFrame();
    }

    device->waitIdle();
}

void VulkanApplication::drawFrame()
{
    gui->render();

    VkResult result;

    result = renderContext->beginFrame();
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        handleSurfaceChange();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    auto& frame = renderContext->getActiveFrame();
    auto syncIndex = renderContext->getSyncIndex();

    updateUniformBuffer(syncIndex);

    frame.getCommandBuffer().reset(0);
    recordCommand(frame.getCommandBuffer(), frame.getRenderTarget(), frame.getFramebuffer(), syncIndex);
    
    renderContext->submit(device->getGraphicsQueue());

    std::vector<VkSwapchainKHR> swapChains = { renderContext->getSwapChain().getHandle() };

    result = renderContext->present(device->getPresentQueue());
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        framebufferResized = false;
        handleSurfaceChange();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    renderContext->endFrame();
}

void VulkanApplication::recordCommand(VulkanCommandBuffer &commandBuffer, const VulkanRenderTarget &renderTarget,
                   const VulkanFramebuffer &framebuffer, uint32_t frameIndex)
{
    commandBuffer.begin(0);

    std::vector<VkClearValue> clearValues{2};
    //clearValues[0].color = {{0.2f, 0.3f, 0.3f, 1.0f}};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    commandBuffer.beginRenderPass(renderTarget, renderContext->getRenderPass(), framebuffer, clearValues, VK_SUBPASS_CONTENTS_INLINE);


    commandBuffer.bindPipeline(renderPipeline->getGraphicsPipeline());

    auto globalDescriptorSetHandle = globalData.descriptorSets[frameIndex]->getHandle();
    vkCmdBindDescriptorSets(commandBuffer.getHandle(),
        renderPipeline->getGraphicsPipeline().getBindPoint(),
        renderPipeline->getPipelineLayout().getHandle(),
        0, 1, &globalDescriptorSetHandle, 0, nullptr);

    auto lightDescriptorSetHandle = lightData.descriptorSets[frameIndex]->getHandle();
    vkCmdBindDescriptorSets(commandBuffer.getHandle(),
        renderPipeline->getGraphicsPipeline().getBindPoint(),
        renderPipeline->getPipelineLayout().getHandle(),
        1, 1, &lightDescriptorSetHandle, 0, nullptr);

    for (const auto& [mesh, id] : renderMeshes) {
        auto& renderMesh = resManager->getRenderMesh(id);
        pushConstants.objId = id;

        vkCmdPushConstants(commandBuffer.getHandle(), renderPipeline->getPipelineLayout().getHandle(),
            VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantRaster), &pushConstants);

        vkCmdBindVertexBuffers(commandBuffer.getHandle(), 0, 1, &renderMesh.vertexBuffer.buffer, &renderMesh.vertexBuffer.offset);

        vkCmdBindIndexBuffer(commandBuffer.getHandle(), renderMesh.indexBuffer.buffer, renderMesh.indexBuffer.offset, renderMesh.indexType);


        commandBuffer.drawIndexed(renderMesh.indexNum, 1, 0, 0, 0);
    }

    gui->renderDrawData(commandBuffer.getHandle());

    commandBuffer.endRenderPass();

    if (commandBuffer.end() != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanApplication::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    static auto lastTime = startTime;

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
    lastTime = currentTime;

    const auto camera = reinterpret_cast<FPSCamera*>(scene->getActiveCamera());
    auto view = camera->calcLookAt();
    view = processInput(window->getHandle(), view, *camera, deltaTime);

    auto extent = renderContext->getSwapChain().getExtent();

    const auto model = scene->getModel("nanosuit");
    model->transComp.translate.x = -20.f;
    model->transComp.rotate.y = time * 90.0f;

    GlobalData ubo{};
    ubo.view = view;
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    auto& buffer = globalData.uniformBuffers[currentImage][1];
    void* bufferData;
    buffer->map(bufferData);
    ObjectData* objData = reinterpret_cast<ObjectData*>(bufferData);
    for (const auto& [mesh, id] : renderMeshes) {
        objData[id].model = mesh->parent->transComp.getTransformMatrix() * mesh->transComp.getTransformMatrix();
    }
    buffer->unmap();

    DirLight* dirLight = scene->getDirLight();
    lightData.uniformBuffers[currentImage][0]->update(dirLight, device->getGPU().pad_uniform_buffer_size(sizeof(DirLight)), 0);

    auto offset = device->getGPU().pad_uniform_buffer_size(sizeof(DirLight));
    std::vector<PointLight> pointLights;
    for (const auto& [name, light] : scene->getPointLightMap()) {
        pointLights.push_back(*light);
    }
    lightData.uniformBuffers[currentImage][0]->update(pointLights.data(), device->getGPU().pad_uniform_buffer_size(sizeof(PointLight) * pointLights.size()), offset);

    pushConstants.lightNum = scene->getPointLightMap().size();
    pushConstants.viewPos = camera->position;
}

glm::mat4 VulkanApplication::processInput(GLFWwindow* window, glm::mat4 view, FPSCamera& camera, float deltaTime)
{
    float speedBuffer = camera.speed;

    static bool isAltPressed = false;

    auto io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            if (!isAltPressed) {
                isAltPressed = true;

                if (!isCursorEnabled) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    isCursorEnabled = true;
                }
                else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    isCursorEnabled = false;
                }
            }
        }
        else {
            isAltPressed = false;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            camera.speed *= 2.0f;
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.move(CameraDirection::FORWARD, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.move(CameraDirection::BACK, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.move(CameraDirection::LEFT, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.move(CameraDirection::RIGHT, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera.move(CameraDirection::HEADUP, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            camera.move(CameraDirection::DOWN, deltaTime);
        }
    }

    view = camera.calcLookAt();
    camera.speed = speedBuffer;

    return view;
}

void VulkanApplication::handleSurfaceChange()
{
    VkExtent2D extent{0, 0};
    extent = window->getExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        extent = window->getExtent();
        glfwWaitEvents();
    }

    device->waitIdle();

    renderContext->recreateSwapChain(extent);

    renderPipeline->recreatePipeline(extent, renderContext->getRenderPass());

    for (auto& [mesh, id] : renderMeshes)
        resManager->getRenderMesh(id).pipeline = &renderPipeline->getGraphicsPipeline();
}

std::vector<const char *> VulkanApplication::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanApplication::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    static double lastX = 400.0f, lastY = 300.0f;
    static double yaw = 0.0f, pitch = 0.0f;

    if (!app->isCursorEnabled) {
        double xoffset = xpos - lastX;
        double yoffset = lastY - ypos;

        auto camera = reinterpret_cast<FPSCamera*>(app->scene->getActiveCamera());
        xoffset *= camera->sensitivity;
        yoffset *= camera->sensitivity;
        camera->rotate((float)xoffset, (float)yoffset);
    }

    lastX = xpos;
    lastY = ypos;
}
