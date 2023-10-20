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

class GraphicsRenderPass
{
public:
    GraphicsRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent);
    ~GraphicsRenderPass();

    virtual void update(float deltaTime, const Scene* scene) {};
    virtual void draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet) = 0;

protected:
    const VulkanDevice& device;
    VulkanResourceManager& resManager;
    VkExtent2D extent;

    std::unique_ptr<VulkanRenderTarget> renderTarget;
    std::unique_ptr<VulkanRenderPass> renderPass;
    std::unique_ptr<VulkanFramebuffer> framebuffer;

    std::unique_ptr<VulkanRenderPipeline> renderPipeline;
};

class SkyboxRenderPass : GraphicsRenderPass
{
public:
    SkyboxRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
        const VulkanRenderPass& graphicsRenderPass);

    void draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet) override;

private:
    const VulkanRenderPass& graphicsRenderPass;
};

class ShadowRenderPass : GraphicsRenderPass
{
public:
    struct ShadowData {
        glm::mat4 lightSpace;
        float bias = 0.0001;
    };

    ShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent);

    void draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet) override;

    constexpr const VulkanImageView* getShadowDepth() const { return shadowDepth; }

private:
    const VulkanImageView* shadowDepth;

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
    std::unique_ptr<SkyboxRenderPass> skyboxPass;
};
