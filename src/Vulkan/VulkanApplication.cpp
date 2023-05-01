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
    threadCount{1}
{
    volkInitialize();

    window = std::make_unique<GlfwWindow>(*this);
    glfwSetCursorPosCallback(window->getHandle(), mouse_callback);

    instance = std::make_unique<VulkanInstance>(getRequiredExtensions(), validationLayers);

    surface = window->createSurface(instance->getHandle());

    device = std::make_unique<VulkanDevice>(instance->getSuitableGPU(surface, deviceExtensions), surface, deviceExtensions, validationLayers);

    resManager = std::make_unique<VulkanResourceManager>(*device, device->getCommandPool());

    scene = std::make_unique<Scene>();
    scene->addModel("nanosuit", "assets/models/nanosuit/nanosuit.obj");
    auto model = scene->getModel("nanosuit");
    model->transComp.translate.z = -20.f;
    model->transComp.translate.y = -5.f;
    scene->addPointLight("light0", { 0.f, 0.f, 10.f }, { 1.0f, 0.f, 0.f });
    scene->addPointLight("light1", { -40.f, 0.f, 10.f }, { 0.0f, 1.f, 0.f });

    VkSampler sampler = resManager->createSampler();
    for (const auto& [name, model] : scene->getModelMap()) {
        for (auto& mesh : model->getMeshes()) {
            std::vector<RenderTexture> textures;
            for (const auto& texture : mesh.textures) {
                textures.push_back({ texture.path.c_str(), sampler });
            }

            renderMeshes.emplace(
                &mesh,
                resManager->requireRenderMesh(mesh.vertices, mesh.indices, mesh.mat, textures)
            );
        }
    }

    graphicBuilder = std::make_unique<VulkanGraphicsBuilder>(*device, *resManager, window->getExtent());

    renderContext = std::make_unique<VulkanRenderContext>(*device, surface, window->getExtent(), threadCount);

    auto vertShader = resManager->createShaderModule("shaders/spv/passthrough.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");

    auto fragShader = resManager->createShaderModule("shaders/spv/post.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(float));
    fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 0);

    renderPipeline = std::make_unique<VulkanRenderPipeline>(*device, *resManager, std::move(vertShader), std::move(fragShader));
    renderPipeline->prepare();
    renderPipeline->getPipelineState().vertexAttributeDescriptions.clear();
    renderPipeline->getPipelineState().vertexBindingDescriptions.clear();
    renderPipeline->getPipelineState().cullMode = VK_CULL_MODE_NONE;
    renderPipeline->getPipelineState().depthStencilState.depth_test_enable = VK_FALSE;
    renderPipeline->getPipelineState().depthStencilState.depth_write_enable = VK_FALSE;
    renderPipeline->recreatePipeline(renderContext->getSwapChain().getExtent(), renderContext->getRenderPass());

    postData = resManager->requireSceneData(*renderPipeline->getDescriptorSetLayouts()[0], threadCount, {});
    for (auto& descSet : postData.descriptorSets) {
        descSet->addWrite(0, VkDescriptorImageInfo{ sampler, graphicBuilder->getOffscreenColor()->getHandle(), VK_IMAGE_LAYOUT_GENERAL });
    }
    postData.update();

    buildRayTracing();

    gui = std::make_unique<GUI>(*instance, *window, *device, renderContext->getRenderPass());
}

VulkanApplication::~VulkanApplication()
{
    resManager.reset();

    gui.reset();

    renderPipeline.reset();
    renderContext.reset();

    graphicBuilder.reset();
    rtBuilder.reset();

    device.reset();

    vkDestroySurfaceKHR(instance->getHandle(), surface, nullptr);
    instance.reset();
    window.reset();
}

void VulkanApplication::buildRayTracing()
{
    rtBuilder = std::make_unique<VulkanRayTracingBuilder>(*device, *resManager, *graphicBuilder->getOffscreenColor());

    // BLAS - Storing each primitive in a geometry
    std::vector<BlasInput> allBlas;
    allBlas.reserve(renderMeshes.size());
    for (const auto& [p_mesh, id] : renderMeshes)
    {
        auto blas = resManager->requireBlasInput(resManager->getRenderMesh(id));

        // We could add more geometry in each BLAS, but we add only one for now
        allBlas.emplace_back(blas);
    }
    rtBuilder->buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

    // TLAS
    std::vector<VkAccelerationStructureInstanceKHR> tlas;
    tlas.reserve(renderMeshes.size());
    uint32_t i = 0;
    for (const auto& [p_mesh, id] : renderMeshes)
    {
        VkAccelerationStructureInstanceKHR rayInst{};
        rayInst.transform = toTransformMatrixKHR(resManager->getRenderMesh(id).tranformMatrix); // Position of the instance
        rayInst.instanceCustomIndex = 0; // gl_InstanceCustomIndexEXT
        rayInst.accelerationStructureReference = rtBuilder->getBlasDeviceAddress(i);
        rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        rayInst.mask = 0xFF; //  Only be hit if rayMask & instance.mask != 0
        rayInst.instanceShaderBindingTableRecordOffset = 0; // We will use the same hit group for all objects
        tlas.emplace_back(rayInst);

        ++i;
    }
    rtBuilder->buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

    std::vector<VulkanShaderModule> rtShaders{};
    rtShaders.emplace_back(resManager->createShaderModule("shaders/spv/raytrace.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR, "main"));
    rtShaders.back().addShaderResourcePushConstant(0, sizeof(PushConstantRayTracing));
    rtShaders.back().addShaderResourceUniform(ShaderResourceType::AccelerationStructure, 0, 0);
    rtShaders.back().addShaderResourceUniform(ShaderResourceType::StorageImage, 0, 1);
    rtShaders.emplace_back(resManager->createShaderModule("shaders/spv/raytrace.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR, "main"));
    rtShaders.back().addShaderResourcePushConstant(0, sizeof(PushConstantRayTracing));
    rtShaders.back().addShaderResourceUniform(ShaderResourceType::AccelerationStructure, 0, 0);
    rtShaders.emplace_back(resManager->createShaderModule("shaders/spv/raytrace.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "main"));
    rtShaders.back().addShaderResourcePushConstant(0, sizeof(PushConstantRayTracing));

    rtBuilder->createRayTracingPipeline(rtShaders, *graphicBuilder->getGlobalData().descSetLayout);
    rtBuilder->createRtShaderBindingTable();
}

void VulkanApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window->getHandle()))
    {
        glfwPollEvents();

        gui->newFrame();
        ImGui::Begin("Debug");
        ImGui::ColorEdit3("Clear color", reinterpret_cast<float*>(&clearColor));
        ImGui::Checkbox("Ray Tracer mode", &useRayTracer);  // Switch between raster and ray tracing

        /*if (ImGui::CollapsingHeader("Light"))
        {
            ImGui::RadioButton("Point", &helloVk.m_pcRaster.lightType, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Infinite", &helloVk.m_pcRaster.lightType, 1);

            ImGui::SliderFloat3("Position", &helloVk.m_pcRaster.lightPosition.x, -20.f, 20.f);
            ImGui::SliderFloat("Intensity", &helloVk.m_pcRaster.lightIntensity, 0.f, 150.f);
        }*/

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

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

    if (!useRayTracer) {
        graphicBuilder->draw(commandBuffer, clearColor);
    }
    else {
        rtBuilder->raytrace(commandBuffer, *graphicBuilder->getGlobalData().descriptorSets[frameIndex], { clearColor });
    }
    
    std::vector<VkClearValue> clearValues{ 2 };
    //clearValues[0].color = {{0.2f, 0.3f, 0.3f, 1.0f}};
    clearValues[0].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
    clearValues[1].depthStencil = { 1.0f, 0 };
    commandBuffer.beginRenderPass(renderTarget, renderContext->getRenderPass(), framebuffer, clearValues, VK_SUBPASS_CONTENTS_INLINE);
    
    commandBuffer.bindPipeline(renderPipeline->getGraphicsPipeline());

    auto extent = renderContext->getSwapChain().getExtent();
    auto aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    vkCmdPushConstants(commandBuffer.getHandle(), 
        renderPipeline->getPipelineLayout().getHandle(), 
        VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &aspectRatio);

    auto postDescSetHandle = postData.descriptorSets[frameIndex][0].getHandle();
    vkCmdBindDescriptorSets(commandBuffer.getHandle(), 
        renderPipeline->getGraphicsPipeline().getBindPoint(), 
        renderPipeline->getPipelineLayout().getHandle(), 0, 1, &postDescSetHandle, 0, nullptr);

    vkCmdDraw(commandBuffer.getHandle(), 3, 1, 0, 0);

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
    model->transComp.translate.z = -20.f;
    model->transComp.rotate.y = time * 90.0f;

    for (const auto& [mesh, id] : renderMeshes) {
        resManager->getRenderMesh(id).tranformMatrix = 
            mesh->parent->transComp.getTransformMatrix() * mesh->transComp.getTransformMatrix();
    }

    graphicBuilder->update(deltaTime, scene.get());
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
