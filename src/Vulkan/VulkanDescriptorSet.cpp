#include "VulkanDevice.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDescriptorSet.h"


VulkanDescriptorSet::VulkanDescriptorSet(
    const VulkanDevice& device,
    VulkanDescriptorPool& descPool,
    const VulkanDescriptorSetLayout& descSetLayout,
    const BindingMap<VkDescriptorBufferInfo>& bufferInfos,
    const BindingMap<VkDescriptorImageInfo>& imageInfos) :

    device{ device }, descSetLayout{ descSetLayout }, descriptorSet{ descPool.allocate(descSetLayout) },
    bufferInfos{ bufferInfos }, imageInfos{ imageInfos }
{
    for (const auto& bindingItem : this->bufferInfos) {
        for (const auto& arrayItem : bindingItem.second) {
            addWrite(bindingItem.first, arrayItem.second, arrayItem.first);
        }
    }

    for (const auto& bindingItem : this->imageInfos) {
        for (const auto& arrayItem : bindingItem.second) {
            addWrite(bindingItem.first, arrayItem.second, arrayItem.first);
        }
    }
}

VulkanDescriptorSet::~VulkanDescriptorSet() {

}

void VulkanDescriptorSet::update()
{
    vkUpdateDescriptorSets(device.getHandle(), toU32(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

VkWriteDescriptorSet VulkanDescriptorSet::makeWrite(uint32_t dstBinding, uint32_t arrayElement)
{
    const auto& bindings = descSetLayout.getBindings();
    const auto& binding = *std::find_if(bindings.begin(), bindings.end(), [=](const auto& b) { return b.binding == dstBinding; });

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = dstBinding;
    descriptorWrite.dstArrayElement = arrayElement;
    descriptorWrite.descriptorType = binding.descriptorType;
    descriptorWrite.descriptorCount = 1;

    return descriptorWrite;
}

void VulkanDescriptorSet::addWrite(uint32_t dstBinding, const VkDescriptorBufferInfo& bufferInfo, uint32_t arrayElement)
{
    bufferInfos[dstBinding][arrayElement] = bufferInfo;
    auto descriptorWrite = makeWrite(dstBinding, arrayElement);
    descriptorWrite.pBufferInfo = &bufferInfos[dstBinding][arrayElement];

    descriptorWrites.push_back(descriptorWrite);
}

void VulkanDescriptorSet::addWrite(uint32_t dstBinding, const VkDescriptorImageInfo& imageInfo, uint32_t arrayElement)
{
    imageInfos[dstBinding][arrayElement] = imageInfo;
    auto descriptorWrite = makeWrite(dstBinding, arrayElement);
    descriptorWrite.pImageInfo = &imageInfos[dstBinding][arrayElement];

    descriptorWrites.push_back(descriptorWrite);
}

void VulkanDescriptorSet::addWrite(uint32_t dstBinding, const VkWriteDescriptorSetAccelerationStructureKHR* as, uint32_t arrayElement)
{
    VkWriteDescriptorSet descriptorWrite = makeWrite(dstBinding, arrayElement);
    descriptorWrite.pNext = as;

    descriptorWrites.push_back(descriptorWrite);
}

VkWriteDescriptorSet VulkanDescriptorSet::makeWriteArray(uint32_t dstBinding, uint32_t count, uint32_t arrayElement)
{
    auto descriptorWrite = makeWrite(dstBinding, arrayElement);
    descriptorWrite.descriptorCount = count;

    return descriptorWrite;
}

void VulkanDescriptorSet::addWriteArray(uint32_t dstBinding, const std::vector<VkDescriptorImageInfo>& imageInfos)
{
    for (size_t i = 0; i < imageInfos.size(); ++i)
        this->imageInfos[dstBinding][i] = imageInfos[i];
    auto descriptorWrite = makeWriteArray(dstBinding, imageInfos.size());
    descriptorWrite.pImageInfo = imageInfos.data();

    descriptorWrites.push_back(descriptorWrite);
}

VkDescriptorSet VulkanDescriptorSet::getHandle() const { return descriptorSet; }
