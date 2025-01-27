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
    const std::vector<VulkanShaderResource> getShaderResources() const;
    const std::vector<VkDescriptorSetLayoutBinding>& getBindings() const;
    VkDescriptorType getType(size_t i) const;
    uint32_t getSetIndex() const;

private:
    uint32_t set;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VulkanShaderResource> shaderResources;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkDescriptorSetLayoutCreateFlags> bindingFlags;

    const VulkanDevice& device;
};