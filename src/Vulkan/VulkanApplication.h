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

struct PushConstantRaster {
    glm::vec3 viewPos;
    int objId;
    int lightNum;
};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, 
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, 
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
};

class VulkanApplication
{
public:
    bool isCursorEnabled = false;
	bool framebufferResized = false;
	bool enableValidationLayers = true;

    VulkanApplication();

    ~VulkanApplication();

    void buildRayTracing();

    void mainLoop();

    void drawFrame();

    void recordCommand(VulkanCommandBuffer& commandBuffer, const VulkanRenderTarget& renderTarget,
        const VulkanFramebuffer& framebuffer, uint32_t frameIndex);

    void updateUniformBuffer(uint32_t currentImage);

    glm::mat4 processInput(GLFWwindow* window, glm::mat4 view, FPSCamera& camera, float deltaTime);

    void handleSurfaceChange();

private:
	std::unique_ptr<GlfwWindow> window;

	std::unique_ptr<VulkanInstance> instance;

	VkSurfaceKHR surface;

	std::unique_ptr<VulkanDevice> device;

    std::unique_ptr<VulkanResourceManager> resManager;
    std::unique_ptr<VulkanRayTracingBuilder> rtBuilder;

    uint32_t threadCount;
	std::unique_ptr<VulkanRenderContext> renderContext;

    std::unique_ptr<VulkanRenderPipeline> renderPipeline;

    std::unique_ptr<Scene> scene;
    
    PushConstantRaster pushConstants;
    SceneData lightData;
    SceneData globalData;
    std::unordered_map<const Mesh*, RenderMeshID> renderMeshes;

    std::unique_ptr<VulkanDescriptorPool> globalDescriptorPool;
    std::unique_ptr<VulkanDescriptorPool> lightDescriptorPool;

    std::unique_ptr<GUI> gui;

    std::vector<const char*> getRequiredExtensions();
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
};
