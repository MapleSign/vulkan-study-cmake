#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"

enum class ShaderResourceType
{
	Sampler,
	Uniform,
    PushConstant,
    StorageBuffer,
    StorageImage,
    AccelerationStructure
};

struct VulkanShaderResource
{
    VkShaderStageFlags stageFlags;
    ShaderResourceType type;

    uint32_t set;
    uint32_t binding;

    uint32_t descriptorCount;

    uint32_t offset;
    uint32_t size;
};

class VulkanShaderModule
{
public:
    VulkanShaderModule(const VulkanDevice& device, const std::vector<char>& code, VkShaderStageFlagBits shaderStageFlag, const char* shaderStageName);
    VulkanShaderModule(const VulkanShaderModule&) = delete;
    VulkanShaderModule(VulkanShaderModule&& other) noexcept;

    ~VulkanShaderModule();

    void addShaderResourceUniform(ShaderResourceType type, uint32_t set, uint32_t binding, uint32_t descriptorCount = 1, VkShaderStageFlags flags = 0);
    void addShaderResourcePushConstant(uint32_t offset, uint32_t size);
    void addShaderResource(VulkanShaderResource res);
    const std::vector<VulkanShaderResource>& getShaderResources() const;

    VkShaderModule getHandle() const;

    VkPipelineShaderStageCreateInfo getShaderStageInfo() const;

private:
    const VulkanDevice& device;

    VkShaderModule shaderModule;
    VkPipelineShaderStageCreateInfo shaderStageInfo;

    std::vector<VulkanShaderResource> shaderResources;
};