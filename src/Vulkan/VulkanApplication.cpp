#include <memory>
#include <vector>
#include <chrono>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Camera.h"
#include "../Platform/GlfwWindow.h"
#include "VulkanInclude.h"
#include "VulkanCommon.h"
#include "VulkanApplication.h"

VulkanApplication::VulkanApplication() :
    threadCount{ 1 }
{
    volkInitialize();

    window = std::make_unique<GlfwWindow>(*this);
    glfwSetCursorPosCallback(window->getHandle(), mouse_callback);
    glfwSetDropCallback(window->getHandle(), drop_callback);
    glfwSetScrollCallback(window->getHandle(), scroll_callback);

    if (enableValidationLayers) {
        try {
            instance = std::make_unique<VulkanInstance>(getRequiredExtensions(), validationLayers);
        }
        catch (const std::exception&) {
            // try without validation layers
            enableValidationLayers = false;
            instance = std::make_unique<VulkanInstance>(getRequiredExtensions(), std::vector<const char*>());
        }
    }
    else {
        instance = std::make_unique<VulkanInstance>(getRequiredExtensions(), std::vector<const char*>());
    }

    surface = window->createSurface(instance->getHandle());

    VulkanPhysicalDevice* gpu;
    auto requiredExtensions = deviceExtensions;
    //rtSupport = false;
    requiredExtensions.insert(requiredExtensions.end(), rtExtensions.begin(), rtExtensions.end());
    try {
        // check if there is a GPU supporting all extensions
        gpu = &instance->getSuitableGPU(surface, requiredExtensions);
    }
    catch (const std::exception&) {
        // if not remove RT and try again
        rtSupport = false;
        useRayTracer = false;
        requiredExtensions = deviceExtensions;
        gpu = &instance->getSuitableGPU(surface, requiredExtensions);
    }
    device = std::make_unique<VulkanDevice>(*gpu, surface, requiredExtensions, validationLayers);

    renderContext = std::make_unique<VulkanRenderContext>(*device, surface, window->getExtent(), threadCount);

    gui = std::make_unique<GUI>(*instance, *window, *device, renderContext->getRenderPass());
}

void VulkanApplication::loadScene(const char* filename)
{
    device->waitIdle();

    resManager.reset();
    scene.reset();
    graphicBuilder.reset();
    rtBuilder.reset();
    renderPipeline.reset();
    renderMeshes.clear();

    resManager = std::make_unique<VulkanResourceManager>(*device, device->getCommandPool());
    resManager->requireTexture("assets/textures/black.jpg", resManager->getDefaultSampler());

    Model* model{ nullptr };
    scene = std::make_unique<Scene>();
    scene->getActiveCamera()->position = { 2.522, 0.90, 0.029 };
    scene->getActiveCamera()->yaw = -182;
    scene->getActiveCamera()->pitch = 0.79;
    reinterpret_cast<FPSCamera*>(scene->getActiveCamera())->rotate(0, 0);
    scene->addModelsFromGltfFile(filename);

    VkSampler sampler = resManager->createSampler();
    for (const auto& [name, model] : scene->getModelMap()) {
        for (auto& mesh : model->getMeshes()) {
            std::vector<RenderTexture> textures;
            for (const auto& texture : mesh.textures) {
                textures.push_back({ texture.type, texture.path.c_str(), sampler });
            }

            auto id = resManager->requireRenderMesh(mesh.vertices, mesh.indices, mesh.mat, textures);
            renderMeshes.emplace(&mesh, id);
            resManager->getRenderMesh(id).tranformMatrix = model->transComp.getTransformMatrix() * mesh.transComp.getTransformMatrix();
        }
    }

    const VkPhysicalDeviceProperties& properties = device->getGPU().getProperties();
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 100.0f;
    auto& cube = scene->loadGLTFFile("assets/models/cube/cube.gltf")[0]->getMeshes()[0];
    resManager->requireSkybox(
        cube.vertices, cube.indices, 
        {
            "assets/textures/skybox/right.jpg",
            "assets/textures/skybox/left.jpg",
            "assets/textures/skybox/top.jpg",
            "assets/textures/skybox/bottom.jpg",
            "assets/textures/skybox/front.jpg",
            "assets/textures/skybox/back.jpg"
        },
        resManager->createSampler(&info));

    graphicBuilder = std::make_unique<VulkanGraphicsBuilder>(*device, *resManager, window->getExtent());

    auto vertShader = resManager->createShaderModule("shaders/spv/passthrough.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");

    auto fragShader = resManager->createShaderModule("shaders/spv/post.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    fragShader.addShaderResourcePushConstant(0, sizeof(PushConstantPost));
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

    if (rtSupport)
        buildRayTracing();
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
        VkGeometryInstanceFlagsKHR flags{};
        if (p_mesh->mat.alphaMode == 0 || (p_mesh->mat.pbrBaseColorFactor.w == 1.0f && p_mesh->mat.pbrBaseColorTexture == -1))
            flags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
        // Need to skip the cull flag in traceray_rtx for double sided materials
        if (p_mesh->mat.doubleSided == 1)
            flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

        VkAccelerationStructureInstanceKHR rayInst{};
        rayInst.transform = toTransformMatrixKHR(resManager->getRenderMesh(id).tranformMatrix); // Position of the instance
        rayInst.instanceCustomIndex = id; // gl_InstanceCustomIndexEXT
        rayInst.accelerationStructureReference = rtBuilder->getBlasDeviceAddress(i);
        rayInst.flags = flags;
        rayInst.mask = 0xFF; //  Only be hit if rayMask & instance.mask != 0
        rayInst.instanceShaderBindingTableRecordOffset = 0; // We will use the same hit group for all objects
        tlas.emplace_back(rayInst);

        ++i;
    }
    rtBuilder->buildTlas(tlas, 
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR);

    std::vector<VulkanShaderModule> rtShaders{};
    rtShaders.emplace_back(resManager->createShaderModule("shaders/spv/raytrace.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR, "main"));
    rtShaders.back().addShaderResourcePushConstant(0, sizeof(PushConstantRayTracing));
    rtShaders.back().addShaderResourceUniform(ShaderResourceType::AccelerationStructure, 0, 0);
    rtShaders.back().addShaderResourceUniform(ShaderResourceType::StorageImage, 0, 1);

    rtShaders.emplace_back(resManager->createShaderModule("shaders/spv/raytrace.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR, "main"));

    rtShaders.emplace_back(resManager->createShaderModule("shaders/spv/raytraceShadow.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR, "main"));
    
    rtShaders.emplace_back(resManager->createShaderModule("shaders/spv/raytrace.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "main"));

    rtShaders.emplace_back(resManager->createShaderModule("shaders/spv/raytrace.rahit.spv", VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "main"));
    rtShaders.back().addShaderResourcePushConstant(0, sizeof(PushConstantRayTracing));
    rtShaders.back().addShaderResourceUniform(ShaderResourceType::AccelerationStructure, 0, 0);
    rtShaders.back().addShaderResourceUniform(ShaderResourceType::StorageImage, 0, 1);

    rtBuilder->createRayTracingPipeline(rtShaders, *graphicBuilder->getGlobalData().descSetLayout, *graphicBuilder->getLightData().descSetLayout);
    rtBuilder->createRtShaderBindingTable();
}

void VulkanApplication::mainLoop()
{
    const char* sceneNames[] = { "cbox" };
    const char* sceneFilePath[] = {
        "assets/models/cbox/cornellBox.gltf"
    };
    int sceneSum = sizeof(sceneNames) / sizeof(char*);

    static int sceneItem = 0;

    loadScene(sceneFilePath[sceneItem]);

    bool enablePostProcessing = false;

    const char* shadowTypeNames[] = { "ShadowMap", "PCF", "PCSS"};
    int shadowTypeNum = sizeof(shadowTypeNames) / sizeof(char*);
   
    while (!glfwWindowShouldClose(window->getHandle()))
    {
        glfwPollEvents();

        bool changed = false;
        bool sceneChanged = false;
        bool cameraChanged = false;

        gui->newFrame();

        ImGui::Begin("Debug");
        changed |= ImGui::ColorEdit3("Clear color", reinterpret_cast<float*>(&clearColor));
        if (rtSupport)
            changed |= ImGui::Checkbox("Ray Tracer mode", &useRayTracer);  // Switch between raster and ray tracing

        if (ImGui::CollapsingHeader("Light"))
        {
            {
                static std::string itemSelected = "";
                std::vector<std::string> deleteList{};
                if (ImGui::BeginListBox("Dir Lights")) {
                    for (auto& [name, light] : scene->getDirLightMap()) {
                        if (ImGui::Selectable(name.c_str(), itemSelected == name)) {
                            itemSelected = name;
                        }
                        if (ImGui::BeginPopupContextItem()) {
                            itemSelected = name;
                            if (ImGui::Button("Delete")) {
                                changed |= true;
                                deleteList.push_back(name);
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }

                        if (itemSelected == name) {
                            changed |= ImGui::SliderFloat3("Direction", &light->direction.x, -1.f, 1.f);
                            changed |= ImGui::SliderFloat("Intensity", &light->intensity, 0.f, 100.f);

                            light->update();
                        }
                    }

                    ImGui::EndListBox();
                }
                if (ImGui::Button("+ Dir Light")) {
                    changed |= true;
                    scene->addDirLight("light");
                }
                for (auto& name : deleteList) {
                    scene->removeDirLight(name.c_str());
                }
            }

            ImGui::Spacing();

            {
                static std::string itemSelected = "";
                std::vector<std::string> deleteList{};
                if (ImGui::BeginListBox("Point Lights")) {
                    for (auto& [name, light] : scene->getPointLightMap()) {
                        if (ImGui::Selectable(name.c_str(), itemSelected == name)) {
                            itemSelected = name;
                        }
                        if (ImGui::BeginPopupContextItem()) {
                            itemSelected = name;
                            if (ImGui::Button("Delete")) {
                                changed |= true;
                                deleteList.push_back(name);
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }

                        if (itemSelected == name) {
                            changed |= ImGui::SliderFloat3("Position", &light->position.x, -20.f, 20.f);
                            changed |= ImGui::SliderFloat("Intensity", &light->intensity, 0.f, 100.f);

                            light->update();
                        }
                    }

                    ImGui::EndListBox();
                }
                if (ImGui::Button("+ Point Light")) {
                    changed |= true;
                    scene->addPointLight("light");
                }
                for (auto& name : deleteList) {
                    scene->removePointLight(name.c_str());
                }
            }
        }

        if (ImGui::CollapsingHeader("Shadow"))
        {
            ImGui::SliderFloat("Bias", &graphicBuilder->getShadowData().bias, 0.0f, 0.005f, "%.4f");
            ImGui::Combo("Shadow Type", &graphicBuilder->getShadowData().shadowType, shadowTypeNames, shadowTypeNum);
            ImGui::SliderInt("PCF Filter Size", &graphicBuilder->getShadowData().pcfFilterSize, 1, 32);
        }

        if (rtSupport)
        {
            if (ImGui::CollapsingHeader("Ray Tracing", ImGuiTreeNodeFlags_DefaultOpen))
            {
                changed |= ImGui::SliderInt("Max Iteration", &maxFrames, 1, 10000);
                changed |= ImGui::SliderInt("Max Ray Depth", &pcRay.maxDepth, 1, 10);
                changed |= ImGui::SliderInt("Samples Per Frame", &pcRay.sampleNumbers, 1, 12);
            }
        }

        if (ImGui::CollapsingHeader("Scenes", ImGuiTreeNodeFlags_DefaultOpen))
        {
            sceneChanged = ImGui::Combo("Scene", &sceneItem, sceneNames, sceneSum);
            changed |= sceneChanged;
        }

        if (ImGui::CollapsingHeader("Camera"))
        {
            cameraChanged |= ImGui::SliderAngle("Defocus Angle", &pcRay.defocusAngle, 0, 179.9);
            cameraChanged |= ImGui::SliderFloat("Focus Dist", &pcRay.focusDist, 0.0001, 20);
            changed |= cameraChanged;
        }

        if (ImGui::CollapsingHeader("Post-processing"))
        {
            ImGui::SliderFloat("Exposure", &pcPost.exposure, 0.0, 10.0);

            const int denoisingAlgorithmSum = 3;
            const char* denoisingAlgorithmStr[denoisingAlgorithmSum] = {
                "Mean filtering",
                "Median filter",
                "Bilateral filter"
            };
            
            ImGui::Checkbox("denoising", &enablePostProcessing);
            ImGui::Combo("Denoising algorithm", &pcPost.denoisingType, denoisingAlgorithmStr, denoisingAlgorithmSum);
            pcPost.enable = enablePostProcessing ? 1 : 0;
            if (pcPost.denoisingType == 2)
            {
                ImGui::Text("Bilateral Filter setting");
                ImGui::SliderFloat("sigma space", &pcPost.sigmaSpace, 0.0, 5);
                ImGui::SliderFloat("sigma color", &pcPost.sigmaColor, 0.0, 100);
            }
        }

        const auto& camera = scene->getActiveCamera();
        pcRay.zFar = camera->zFar;

        if (changed) {
            resetFrameCount();
        }
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Frame index %d", pcRay.frame);
        ImGui::End();

        drawFrame();

        if (sceneChanged) {
            loadScene(sceneFilePath[sceneItem]);
            continue;
        }
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
    //updateTlas();

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

void VulkanApplication::resetFrameCount()
{
    pcRay.frame = -1;
}

void VulkanApplication::updateFrameCount()
{
    static glm::mat4 refCamMatrix;
    // TODO: need fov?
    const glm::mat4& m = scene.get()->getActiveCamera()->calcLookAt();

    if (memcmp(&refCamMatrix, &m, sizeof(glm::mat4)) != 0)
    {
        resetFrameCount();
        refCamMatrix = m;
    }

    pcRay.frame++;
}

void VulkanApplication::recordCommand(VulkanCommandBuffer &commandBuffer, const VulkanRenderTarget &renderTarget,
    const VulkanFramebuffer &framebuffer, uint32_t frameIndex)
{
    commandBuffer.begin(0);

    if (!rtSupport || !useRayTracer) {
        graphicBuilder->draw(commandBuffer, clearColor);
    }
    else if (rtSupport) {
        updateFrameCount();
        if (pcRay.frame < maxFrames) {
            pcRay.clearColor = clearColor;
            rtBuilder->raytrace(commandBuffer, 
                *graphicBuilder->getGlobalData().descriptorSets[frameIndex], 
                *graphicBuilder->getLightData().descriptorSets[frameIndex], 
                pcRay);
        }
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
        VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pcPost), &pcPost);

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

    for (const auto& [mesh, id] : renderMeshes) {
        resManager->getRenderMesh(id).tranformMatrix = 
            mesh->parent->transComp.getTransformMatrix() * mesh->transComp.getTransformMatrix();
    }

    graphicBuilder->update(deltaTime, scene.get());

    pcRay.dirLightNum = scene->getDirLightMap().size();
    pcRay.pointLightNum = scene->getPointLightMap().size();
}

void VulkanApplication::updateTlas()
{
    std::vector<VkAccelerationStructureInstanceKHR> tlas;
    tlas.reserve(renderMeshes.size());
    uint32_t i = 0;
    for (const auto& [p_mesh, id] : renderMeshes)
    {
        VkGeometryInstanceFlagsKHR flags{};
        if (p_mesh->mat.alphaMode == 0 || (p_mesh->mat.pbrBaseColorFactor.w == 1.0f && p_mesh->mat.pbrBaseColorTexture == -1))
            flags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
        // Need to skip the cull flag in traceray_rtx for double sided materials
        if (p_mesh->mat.doubleSided == 1)
            flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

        VkAccelerationStructureInstanceKHR rayInst{};
        rayInst.transform = toTransformMatrixKHR(resManager->getRenderMesh(id).tranformMatrix); // Position of the instance
        rayInst.instanceCustomIndex = id; // gl_InstanceCustomIndexEXT
        rayInst.accelerationStructureReference = rtBuilder->getBlasDeviceAddress(i);
        rayInst.flags = flags;
        rayInst.mask = 0xFF; //  Only be hit if rayMask & instance.mask != 0
        rayInst.instanceShaderBindingTableRecordOffset = 0; // We will use the same hit group for all objects
        tlas.emplace_back(rayInst);

        ++i;
    }
    rtBuilder->buildTlas(
        tlas,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR, 
        true
    );
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

    graphicBuilder->recreateGraphicsBuilder(extent);

    if (rtSupport)
        rtBuilder->recreateRayTracingBuilder(*(graphicBuilder->getOffscreenColor()));

    renderContext->recreateSwapChain(extent);

    renderPipeline->recreatePipeline(extent, renderContext->getRenderPass());

    for (auto& [mesh, id] : renderMeshes)
        resManager->getRenderMesh(id).pipeline = &renderPipeline->getGraphicsPipeline();

    VkSampler sampler = resManager->createSampler();
    postData = resManager->requireSceneData(*renderPipeline->getDescriptorSetLayouts()[0], threadCount, {});
    for (auto& descSet : postData.descriptorSets) {
        descSet->addWrite(0, VkDescriptorImageInfo{ sampler, graphicBuilder->getOffscreenColor()->getHandle(), VK_IMAGE_LAYOUT_GENERAL });
    }
    postData.update();

    resetFrameCount();
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

    

    if (!app->isCursorEnabled|| ImGui::GetIO().WantCaptureMouse==false) {
        int leftButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        int rightButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
        int midButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);

        double xoffset = xpos - lastX;
        double yoffset = lastY - ypos;

        auto camera = reinterpret_cast<FPSCamera*>(app->scene->getActiveCamera());
        xoffset *= camera->sensitivity;
        yoffset *= camera->sensitivity;

        const double moveSensitivity = 0.1;

        if (leftButtonState)
        {
            camera->rotate((float)xoffset, (float)yoffset);
        }
        else if (rightButtonState)
        {
            float offset = sqrt(xoffset * xoffset + yoffset * yoffset)* moveSensitivity;
            if (xoffset + yoffset > 0)
            {
                camera->BaseCamera::move(CameraDirection::FORWARD, offset);
            }
            else
            {
                camera->BaseCamera::move(CameraDirection::BACK, offset);
            }
        }
        else if (midButtonState)
        {
            camera->BaseCamera::move(CameraDirection::RIGHT, xoffset* moveSensitivity);
            camera->BaseCamera::move(CameraDirection::HEADUP, yoffset* moveSensitivity);
        }
    }

    lastX = xpos;
    lastY = ypos;
}

void VulkanApplication::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    const float SCROLL_SENSITIVITY = 0.3;
    auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    auto camera = reinterpret_cast<FPSCamera*>(app->scene->getActiveCamera());

    if (ImGui::GetIO().WantCaptureMouse == false)
    {
        camera->BaseCamera::move(CameraDirection::FORWARD, yoffset* SCROLL_SENSITIVITY);
    }

}

void VulkanApplication::drop_callback(GLFWwindow* window, int count, const char** path)
{
    auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    for (int i = 0; i < count; i++)
    {
        std::cout << path[i] << std::endl;
        std::string pathNew(path[i]);
        for (int j = 0; j < strlen(path[i]); j++)
        {
            if (path[i][j] == '\\')
            {
                pathNew[j] = '/';
            }
        }
        std::cout << pathNew << std::endl;
        app->loadScene(pathNew.c_str());
    }
    //if (count == 1)
    //{
    //    app->loadScene(path[0]);
    //}
}