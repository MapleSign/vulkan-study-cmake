#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanShaderModule.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanPipelineLayout.h"
#include "VulkanGraphicsPipeline.h"

class VulkanResourceManager;
class VulkanRenderPass;

class VulkanSubpass
{
public:
    VulkanSubpass(const VulkanDevice& device, VulkanResourceManager& resManager,
    VulkanShaderModule&& vertShader, VulkanShaderModule&& fragShader);

    ~VulkanSubpass();

    virtual void prepare() = 0;
private:
    const VulkanDevice& device;
    VulkanResourceManager& resManager;
    const VulkanRenderPass* renderPass{ nullptr };

	VulkanShaderModule vertShader;
	VulkanShaderModule fragShader;
    
    VulkanPipelineLayout* pipelineLayout{ nullptr };
    std::vector<VulkanDescriptorSetLayout*> descSetLayouts{};
    VulkanGraphicsPipeline* pipeline{ nullptr };
};
