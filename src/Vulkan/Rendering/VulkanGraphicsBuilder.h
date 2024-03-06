#pragma once

#include "Scene.h"

#include "VulkanDevice.h"
#include "VulkanResource.h"
#include "VulkanImageView.h"
#include "VulkanRenderTarget.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderPipeline.h"

class GlobalSubpass;
class LightingSubpass;

struct ShadowData {
    int shadowType = 1;
    int pcfFilterSize = 8;

    float bias = 0.0001;
    int maxDirShadowNum = 2;

    int maxPointShadowNum = 2;

    glm::vec3 padding;
};

struct PushConstantRaster {
    glm::vec3 viewPos;
    int objId;
    int dirLightNum;
    int pointLightNum;
};

enum GBufferType {
    SceneColor = 0,
    Position,
    Albedo,
    Normal,
    MetalRough,

    Count,

    Color = Count,
    Depth
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
        const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& graphicsRenderPass, uint32_t subpass);

    void update(float deltaTime, const Scene* scene) override;
    void draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet) override;

private:
    const VulkanRenderPass& graphicsRenderPass;
};

class LightingRenderPass : GraphicsRenderPass 
{
public:
    LightingRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
        const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& graphicsRenderPass, uint32_t subpass, 
        const std::vector<const VulkanImageView*>& gBuffer);

    void update(float deltaTime, const Scene* scene) override;
    void draw(VulkanCommandBuffer& cmdBuf, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet) override;

private:
    const VulkanRenderPass& graphicsRenderPass;

    SceneData defferedData;

    PushConstantRaster pushConstants{};
};

class ShadowRenderPass : public GraphicsRenderPass
{
public:
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

    constexpr const SceneData& getGlobalData() const;
    constexpr const SceneData& getLightData() const;


    ShadowData& getShadowData() { return shadowData; }

private:
    void createRenderTarget();
    void createRenderPass();

    const VulkanDevice& device;
    VulkanResourceManager& resManager;
    VkExtent2D extent;

    std::vector<const VulkanImageView*> gBuffer;
    const VulkanImageView* offscreenColor;
    const VulkanImageView* offscreenDepth;

    std::unique_ptr<VulkanRenderTarget> renderTarget;
    std::unique_ptr<VulkanRenderPass> renderPass;
    std::unique_ptr<VulkanFramebuffer> framebuffer;

    ShadowData shadowData{};

    std::unique_ptr<DirShadowRenderPass> dirShadowPass;
    std::unique_ptr<PointShadowRenderPass> pointShadowPass;

    std::unique_ptr<GlobalSubpass> globalPass;
    std::unique_ptr<SkyboxRenderPass> skyboxPass;
    std::unique_ptr<LightingSubpass> lightingPass;
};
