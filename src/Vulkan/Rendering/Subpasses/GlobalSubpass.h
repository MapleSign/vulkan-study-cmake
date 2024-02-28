#pragma once

#include "Rendering/VulkanSubpass.h"

#include "Rendering/VulkanGraphicsBuilder.h"

class GlobalSubpass : public VulkanSubpass
{
public:
    GlobalSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
        const VulkanRenderPass& renderPass, uint32_t subpass);
    ~GlobalSubpass();

    void prepare(
        const std::vector<std::unique_ptr<VulkanImageView>>& dirLightShadowMaps,
        const std::vector<std::unique_ptr<VulkanImageView>>& pointLightShadowMaps);

    void update(float deltaTime, const Scene* scene) override;
    void update(float deltaTime, const Scene* scene, ShadowData shadowData);
    void draw(VulkanCommandBuffer& cmdBuf, const std::vector<VulkanDescriptorSet>& globalSets) override;

    constexpr const SceneData& getGlobalData() const { return globalData; }
    constexpr const SceneData& getLightData() const { return lightData; }

private:
    SceneData globalData;
    SceneData lightData;

    PushConstantRaster pushConstants{};
};
