#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanShaderModule.h"

VulkanShaderModule::VulkanShaderModule(const VulkanDevice &device, const std::vector<char> &code, VkShaderStageFlagBits shaderStageFlag, const char *shaderStageName)
    : device{ device }, shaderModule{}, shaderStageInfo{}
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(device.getHandle(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = shaderStageFlag;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = shaderStageName;
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept :
    device{ other.device }, shaderModule{ other.shaderModule }, shaderStageInfo{ other.shaderStageInfo }, shaderResources{ other.shaderResources }
{
    other.shaderModule = VK_NULL_HANDLE;
}

VulkanShaderModule::~VulkanShaderModule() {
    if (shaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(device.getHandle(), shaderModule, nullptr);
}

void VulkanShaderModule::addShaderResourceUniform(ShaderResourceType type, uint32_t set, uint32_t binding, uint32_t descriptorCount, VkShaderStageFlags flags)
{
    VulkanShaderResource res;
    res.stageFlags = shaderStageInfo.stage | flags;
    res.type = type;
    res.set = set;
    res.binding = binding;
    res.descriptorCount = descriptorCount;
    shaderResources.emplace_back(std::move(res));
}

void VulkanShaderModule::addShaderResourcePushConstant(uint32_t offset, uint32_t size)
{
    VulkanShaderResource res;
    res.stageFlags = shaderStageInfo.stage;
    res.type = ShaderResourceType::PushConstant;
    res.offset = offset;
    res.size = size;
    shaderResources.emplace_back(std::move(res));
}

void VulkanShaderModule::addShaderResource(VulkanShaderResource res)
{
    shaderResources.push_back(res);
}

const std::vector<VulkanShaderResource>& VulkanShaderModule::getShaderResources() const
{
    return shaderResources;
}

VkShaderModule VulkanShaderModule::getHandle() const {
    return shaderModule;
}

VkPipelineShaderStageCreateInfo VulkanShaderModule::getShaderStageInfo() const {
    return shaderStageInfo;
}
