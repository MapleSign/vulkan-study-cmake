#pragma once

#include <array>

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanShaderModule.h"

VkDescriptorType findDescriptorType(ShaderResourceType type);

class VulkanDescriptorSetLayout
{
public:
    VulkanDescriptorSetLayout(const VulkanDevice& device, uint32_t set, const std::vector<VulkanShaderResource>& shaderResources);
    VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) noexcept;
    ~VulkanDescriptorSetLayout();

    const VkDescriptorSetLayout& getHandle() const;
    const std::vector<VkDescriptorSetLayoutBinding>& getBindings() const;
    uint32_t getSetIndex() const;

private:
    uint32_t set;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    const VulkanDevice& device;
};