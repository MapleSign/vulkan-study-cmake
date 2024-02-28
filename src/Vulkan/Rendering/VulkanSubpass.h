#pragma once

#include "Scene.h"

#include "VulkanDevice.h"
#include "VulkanResource.h"
#include "VulkanImageView.h"
#include "VulkanRenderTarget.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderPipeline.h"

class VulkanSubpass
{
public:
    VulkanSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent,
        const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& renderPass, uint32_t subpass);
    ~VulkanSubpass();

    virtual void prepare() {};
    virtual void update(float deltaTime, const Scene* scene) = 0;
    virtual void draw(VulkanCommandBuffer& cmdBuf, const std::vector<VulkanDescriptorSet>& globalSets) = 0;

    virtual const VulkanRenderPipeline& getRenderPipeline() const { return *renderPipeline; }
    virtual void recreatePipeline(const VkExtent2D extent, const VulkanRenderPass& renderPass) 
    { 
        renderPipeline->recreatePipeline(extent, renderPass); 
    }

    virtual std::vector<VulkanShaderResource> getShaderResources() const {
        std::vector<VulkanShaderResource> shaderResources{};
        for (const auto& descLayout : renderPipeline->getDescriptorSetLayouts()) {
            const auto& descShaderRes = descLayout->getShaderResources();
            shaderResources.insert(shaderResources.end(), descShaderRes.begin(), descShaderRes.end());
        }
        return shaderResources;
    }

protected:
    const VulkanDevice& device;
    VulkanResourceManager& resManager;
    VkExtent2D extent;

    const VulkanRenderPass& renderPass;

    std::unique_ptr<VulkanRenderPipeline> renderPipeline;
};
