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

void VulkanShaderModule::addShaderResourceUniform(ShaderResourceType type, uint32_t set, uint32_t binding, 
    uint32_t descriptorCount, VkShaderStageFlags flags, ShaderResourceMode mode)
{
    VulkanShaderResource res;
    res.stageFlags = shaderStageInfo.stage | flags;
    res.type = type;
    res.mode = mode;
    res.set = set;
    res.binding = binding;
    res.descriptorCount = descriptorCount;
    shaderResources.emplace_back(std::move(res));
}

void VulkanShaderModule::addShaderResourcePushConstant(uint32_t offset, uint32_t size, VkShaderStageFlags flags)
{
    VulkanShaderResource res;
    res.stageFlags = shaderStageInfo.stage | flags;
    res.type = ShaderResourceType::PushConstant;
    res.offset = offset;
    res.size = size;
    shaderResources.emplace_back(std::move(res));
}

void VulkanShaderModule::addShaderResource(VulkanShaderResource res)
{
    shaderResources.push_back(res);
}

void VulkanShaderModule::addShaderResources(const std::vector<VulkanShaderResource>& resources)
{
    shaderResources.insert(shaderResources.end(), resources.begin(), resources.end());
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

void createLayoutInfo(
    const std::vector<VulkanShaderResource>& resources, 
    std::vector<VkPushConstantRange>& pushConstantRanges, 
    std::map<uint32_t, std::vector<VulkanShaderResource>>& descriptorResourceSets)
{
    for (const auto& res : resources) {
        if (res.type == ShaderResourceType::PushConstant) {
            auto it = std::find_if(pushConstantRanges.begin(), pushConstantRanges.end(),
                [&](auto& pcRange) { return pcRange.offset == res.offset; });

            if (it != pushConstantRanges.end())
                it->stageFlags |= res.stageFlags;
            else
                pushConstantRanges.push_back({ res.stageFlags, res.offset, res.size });
        }
        else {
            if (descriptorResourceSets.end() != descriptorResourceSets.find(res.set)) {
                auto it = std::find_if(descriptorResourceSets[res.set].begin(), descriptorResourceSets[res.set].end(),
                    [&](auto& r) { return r.binding == res.binding; });

                if (it != descriptorResourceSets[res.set].end()) {
                    it->stageFlags |= res.stageFlags;
                    continue;
                }
            }

            descriptorResourceSets[res.set].push_back(res);
        }
    }
}
