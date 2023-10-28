#pragma once

#include <unordered_map>

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

enum class ShaderResourceMode
{
    Static,
    Dynamic,
    UpdateAfterBind
};

struct VulkanShaderResource
{
    VkShaderStageFlags stageFlags;
    ShaderResourceType type;
    ShaderResourceMode mode{ ShaderResourceMode::Static };

    uint32_t set;
    uint32_t binding;

    uint32_t descriptorCount;

    uint32_t offset;
    uint32_t size;
};

void createLayoutInfo(
    const std::vector<VulkanShaderResource>& resources, 
    std::vector<VkPushConstantRange>& pushConstantRanges, 
    std::unordered_map<uint32_t, std::vector<VulkanShaderResource>>& descriptorResourceSets);

class VulkanShaderModule
{
public:
    VulkanShaderModule(const VulkanDevice& device, const std::vector<char>& code, VkShaderStageFlagBits shaderStageFlag, const char* shaderStageName);
    VulkanShaderModule(const VulkanShaderModule&) = delete;
    VulkanShaderModule(VulkanShaderModule&& other) noexcept;

    ~VulkanShaderModule();

    void addShaderResourceUniform(ShaderResourceType type, uint32_t set, uint32_t binding, 
        uint32_t descriptorCount = 1, VkShaderStageFlags flags = 0, ShaderResourceMode mode = { ShaderResourceMode::Static });
    void addShaderResourcePushConstant(uint32_t offset, uint32_t size, VkShaderStageFlags flags = 0);
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