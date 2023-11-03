#include <array>

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanShaderModule.h"
#include "VulkanDescriptorSetLayout.h"

VkDescriptorType findDescriptorType(ShaderResourceType type)
{
    switch (type)
    {
    case ShaderResourceType::Input:
        return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        break;
    case ShaderResourceType::Sampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        break;
    case ShaderResourceType::Uniform:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
    case ShaderResourceType::StorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        break;
    case ShaderResourceType::StorageImage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        break;
    case ShaderResourceType::AccelerationStructure:
        return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        break;
    case ShaderResourceType::PushConstant:
    default:
        throw std::runtime_error("no corresponding descriptor type!");
        break;
    }
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const VulkanDevice& device, uint32_t set, const std::vector<VulkanShaderResource>& shaderResources) :
    device{ device }, set{ set }, shaderResources{ shaderResources }
{
    bindings.resize(shaderResources.size());
    bindingFlags.resize(shaderResources.size());
    for (const auto& res : shaderResources) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = res.binding;
        layoutBinding.descriptorCount = res.descriptorCount;
        layoutBinding.descriptorType = findDescriptorType(res.type);
        layoutBinding.pImmutableSamplers = nullptr;
        layoutBinding.stageFlags = res.stageFlags;

        bindings[res.binding] = layoutBinding; // we assume that bindings are always from 0 to max binding.(continuous)
        bindingFlags[res.binding] = res.mode == ShaderResourceMode::UpdateAfterBind ? 
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT :
            0;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlagsInfo{};
    layoutBindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    layoutBindingFlagsInfo.bindingCount = toU32(bindingFlags.size());
    layoutBindingFlagsInfo.pBindingFlags = bindingFlags.data();

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.bindingCount = toU32(bindings.size());
    layoutInfo.pBindings = bindings.data();
    layoutInfo.pNext = &layoutBindingFlagsInfo;

    if (vkCreateDescriptorSetLayout(device.getHandle(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) noexcept :
    device{ other.device }, set{ other.set },
    shaderResources{ other.shaderResources } , bindings{ other.bindings }, bindingFlags{ other.bindingFlags },
    descriptorSetLayout{ other.descriptorSetLayout }
{
    other.descriptorSetLayout = VK_NULL_HANDLE;
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.getHandle(), descriptorSetLayout, nullptr);
    }
}

const VkDescriptorSetLayout &VulkanDescriptorSetLayout::getHandle() const { return descriptorSetLayout; }

const std::vector<VulkanShaderResource> VulkanDescriptorSetLayout::getShaderResources() const { return shaderResources; }

const std::vector<VkDescriptorSetLayoutBinding>& VulkanDescriptorSetLayout::getBindings() const { return bindings; }

VkDescriptorType VulkanDescriptorSetLayout::getType(size_t i) const
{
    return bindings[i].descriptorType;
}

uint32_t VulkanDescriptorSetLayout::getSetIndex() const
{
    return set;
}
