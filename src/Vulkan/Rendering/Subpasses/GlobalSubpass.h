#pragma once

#include "Rendering/VulkanSubpass.h"

class GlobalSubpass : public VulkanSubpass
{
public:
    GlobalSubpass(const VulkanDevice& device, VulkanResourceManager& resManager,
        VulkanShaderModule&& vertShader, VulkanShaderModule&& fragShader, 
        const VulkanDescriptorSetLayout* globalDescSetLayout);
    ~GlobalSubpass();

    virtual void prepare() override;
private:
    const VulkanDescriptorSetLayout* globalDescSetLayout{ nullptr };
};
