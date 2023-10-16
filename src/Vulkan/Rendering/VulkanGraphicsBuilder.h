#pragma once

#include "Scene.h"

#include "VulkanDevice.h"
#include "VulkanResource.h"
#include "VulkanImageView.h"
#include "VulkanRenderTarget.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderPipeline.h"

struct PushConstantRaster {
    glm::vec3 viewPos;
    int objId;
    int lightNum;
};

class ShadowRenderPass
{
public:
    struct ShadowData {
        glm::mat4 lightSpace;
        float bias = 0.0001;
    };

    ShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent);
    ~ShadowRenderPass();

    void update(float deltaTime, const Scene* scene);
    void draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet);

    constexpr const VulkanImageView* getShadowDepth() const { return shadowDepth; }

private:
    const VulkanDevice& device;
    VulkanResourceManager& resManager;
    VkExtent2D extent;

    const VulkanImageView* shadowDepth;

    std::unique_ptr<VulkanRenderTarget> renderTarget;
    std::unique_ptr<VulkanRenderPass> renderPass;
    std::unique_ptr<VulkanFramebuffer> framebuffer;

    std::unique_ptr<VulkanRenderPipeline> renderPipeline;

    PushConstantRaster pushConstants{};
};

class VulkanGraphicsBuilder
{
public:
    VulkanGraphicsBuilder(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent);
    ~VulkanGraphicsBuilder();

    void recreateGraphicsBuilder(const VkExtent2D extent);

    void update(float deltaTime, const Scene* scene);
    void draw(VulkanCommandBuffer& cmdBuf, glm::vec4 clearColor);

    constexpr const VulkanImageView* getOffscreenColor() const { return offscreenColor; }
    constexpr const VulkanImageView* getOffscreenDepth() const { return offscreenDepth; }

    constexpr const SceneData& getGlobalData() const { return globalData; }
    constexpr const SceneData& getLightData() const { return lightData; }

private:
    const VulkanDevice& device;
    VulkanResourceManager& resManager;
    VkExtent2D extent;

    const VulkanImageView* offscreenColor;
    const VulkanImageView* offscreenDepth;

    std::unique_ptr<VulkanRenderTarget> renderTarget;
    std::unique_ptr<VulkanRenderPass> renderPass;
    std::unique_ptr<VulkanFramebuffer> framebuffer;

    std::unique_ptr<VulkanRenderPipeline> renderPipeline;
    SceneData globalData;
    SceneData lightData;

    PushConstantRaster pushConstants{};

    std::unique_ptr<ShadowRenderPass> shadowPass;
};
