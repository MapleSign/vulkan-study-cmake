#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include <unordered_set>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Scene.h"
#include "../Camera.h"
#include "../Model.h"
#include "VulkanInclude.h"
#include "../GUI/GUI.h"

class GlfwWindow;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_KHR_SHADER_CLOCK_EXTENSION_NAME
};

const std::vector<const char*> rtExtensions = {
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
};

class VulkanApplication
{
public:
    bool isCursorEnabled = true;
	bool framebufferResized = false;
	bool enableValidationLayers = true;
    bool rtSupport = true;

    VulkanApplication();

    ~VulkanApplication();

    void loadScene(const char* filename);



    void buildRayTracing();

    void mainLoop();

    void drawFrame();

    void resetFrameCount();

    void updateFrameCount();

    void recordCommand(VulkanCommandBuffer& commandBuffer, const VulkanRenderTarget& renderTarget,
        const VulkanFramebuffer& framebuffer, uint32_t frameIndex);

    void updateUniformBuffer(uint32_t currentImage);
    void updateTlas();

    glm::mat4 processInput(GLFWwindow* window, glm::mat4 view, FPSCamera& camera, float deltaTime);

    void handleSurfaceChange();

private:
	std::unique_ptr<GlfwWindow> window;

	std::unique_ptr<VulkanInstance> instance;

	VkSurfaceKHR surface;

	std::unique_ptr<VulkanDevice> device;

    std::unique_ptr<VulkanResourceManager> resManager;
    std::unique_ptr<VulkanRayTracingBuilder> rtBuilder;
    std::unique_ptr<VulkanGraphicsBuilder> graphicBuilder;

    uint32_t threadCount;
	std::unique_ptr<VulkanRenderContext> renderContext;

    std::unique_ptr<VulkanRenderPipeline> renderPipeline;

    std::unique_ptr<Scene> scene;
    
    SceneData postData;
    std::unordered_map<const Mesh*, RenderMeshID> renderMeshes;

    std::unique_ptr<GUI> gui;
    bool useRayTracer = false;
    glm::vec4 clearColor{ 0.5f, 0.8f, 0.9f, 1.0f };
    PushConstantRayTracing pcRay{ {}, {-0.029, -1.00, -0.058}, {3.f}, 1, -1, 5, 1 };

    std::vector<const char*> getRequiredExtensions();
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

    int maxFrames{ 10000 };
};
