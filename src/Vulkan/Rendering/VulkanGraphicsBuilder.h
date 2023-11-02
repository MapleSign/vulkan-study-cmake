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
    int dirLightNum;
    int pointLightNum;
};

class GraphicsRenderPass
{
public:
    GraphicsRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
        const std::vector<VulkanShaderResource> shaderRes = {});
    ~GraphicsRenderPass();

    virtual void update(float deltaTime, const Scene* scene) = 0;
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
        const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& graphicsRenderPass);

    void update(float deltaTime, const Scene* scene) override;
    void draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet) override;

private:
    const VulkanRenderPass& graphicsRenderPass;
};

class ShadowRenderPass : public GraphicsRenderPass
{
public:
    struct ShadowData {
        glm::mat4 lightSpace;

        float bias = 0.0001;
        int maxDirShadowNum = 2;
        int maxPointShadowNum = 2;
    };

    ShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
        const std::vector<VulkanShaderResource> shaderRes, uint32_t maxLightNum);

    virtual void update(float deltaTime, const Scene* scene) override;
    virtual void draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet) override;

    constexpr const std::vector<std::unique_ptr<VulkanImageView>>& getShadowDepths() const { return shadowDepths; }

protected:
    uint32_t maxLightNum;

    std::vector<std::unique_ptr<VulkanImageView>> shadowDepths;

    PushConstantRaster pushConstants{};
};

class DirShadowRenderPass : public ShadowRenderPass
{
public:
    DirShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
        const std::vector<VulkanShaderResource> shaderRes, uint32_t maxLightNum);
};

class PointShadowRenderPass : public ShadowRenderPass
{
public:
    PointShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
        const std::vector<VulkanShaderResource> shaderRes, uint32_t maxLightNum);
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


    ShadowRenderPass::ShadowData& getShadowData() { return shadowData; }

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
    ShadowRenderPass::ShadowData shadowData{};

    std::unique_ptr<DirShadowRenderPass> dirShadowPass;
    std::unique_ptr<PointShadowRenderPass> pointShadowPass;
    std::unique_ptr<SkyboxRenderPass> skyboxPass;
};
