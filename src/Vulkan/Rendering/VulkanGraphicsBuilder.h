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
class SkyboxSubpass;
class SSAOSubpass;
class SSAOBlurSubpass;

struct GraphicsLightInfo {
    std::vector<DirLight> dirLights;
    std::vector<PointLight> pointLights;
};

struct GraphicsRenderingInfo {
    GraphicsLightInfo lightInfo;
};

struct ShadowData {
    int shadowType = 1;
    int pcfFilterSize = 8;

    int pcssBlockerSize = 2;
    float bias = 0.0001;

    int maxDirShadowNum = 2;
    int maxPointShadowNum = 2;

    glm::vec2 padding;
};

enum LightType {
    LIGHT_TYPE_DIR = 0,
    LIGHT_TYPE_POINT = 1,
    LIGHT_TYPE_NONE = -1,
};

// Push constant structure for the raster
struct PushConstantShadow
{
	int objId;
    int lightType; // 0: dir, 1: point, -1: none - used for shadow with geom shader
    int lightId; // dir id or point id
	int layerId; // csm level for dir; face id for point

    glm::vec3 padding;
    int lightNum;
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
    Normal,
    Albedo,
    MetalRough,
    SSAO,

    Count,

    Color = Count,
    Depth,

    Tmp,

    Total
};

class GraphicsRenderPass
{
public:
    GraphicsRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
        const std::vector<VulkanShaderResource> shaderRes = {});
    ~GraphicsRenderPass();

    virtual void update(float deltaTime, const Scene* scene) = 0;
    virtual void draw(VulkanCommandBuffer& cmdBuf, const GraphicsRenderingInfo& renderingInfo, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet) = 0;

protected:
    const VulkanDevice& device;
    VulkanResourceManager& resManager;
    VkExtent2D extent;

    std::unique_ptr<VulkanRenderTarget> renderTarget;
    std::unique_ptr<VulkanRenderPass> renderPass;
    std::unique_ptr<VulkanFramebuffer> framebuffer;

    std::unique_ptr<VulkanRenderPipeline> renderPipeline;
};

class ShadowRenderPass : public GraphicsRenderPass
{
public:
    ShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
        const std::vector<VulkanShaderResource> shaderRes, uint32_t maxLightNum);

    virtual void update(float deltaTime, const Scene* scene) override;
    virtual void draw(VulkanCommandBuffer& cmdBuf, const GraphicsRenderingInfo& renderingInfo, const VulkanDescriptorSet& globalSet, const VulkanDescriptorSet& lightSet) override;

    constexpr const std::vector<std::unique_ptr<VulkanImageView>>& getShadowDepths() const { return shadowDepths; }

protected:
    uint32_t maxLightNum;

    std::vector<std::unique_ptr<VulkanFramebuffer>> shadowFramebuffers;
    std::vector<std::unique_ptr<VulkanRenderTarget>> shadowRenderTargets;
    std::vector<std::unique_ptr<VulkanImage>> shadowImages;
    std::vector<std::unique_ptr<VulkanImageView>> shadowDepths;

    PushConstantShadow pushConstants{};
};

class DirShadowRenderPass : public ShadowRenderPass
{
public:
    DirShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
        const std::vector<VulkanShaderResource> shaderRes, uint32_t maxLightNum, uint32_t maxCSMLevel);

    virtual void update(float deltaTime, const Scene* scene) override;
private:
    uint32_t maxCSMLevel;
};

class PointShadowRenderPass : public ShadowRenderPass
{
public:
    PointShadowRenderPass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
        const std::vector<VulkanShaderResource> shaderRes, uint32_t maxLightNum);

    virtual void update(float deltaTime, const Scene* scene) override;
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

    GraphicsRenderingInfo renderingInfo{};
    ShadowData shadowData{};

    std::unique_ptr<DirShadowRenderPass> dirShadowPass;
    std::unique_ptr<PointShadowRenderPass> pointShadowPass;

    std::unique_ptr<GlobalSubpass> globalPass;
    std::unique_ptr<SkyboxSubpass> skyboxPass;
    std::unique_ptr<LightingSubpass> lightingPass;

    std::unique_ptr<SSAOSubpass> ssaoPass;
    std::unique_ptr<SSAOBlurSubpass> ssaoBlurPass;
};
