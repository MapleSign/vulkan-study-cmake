#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

VulkanBuffer::VulkanBuffer(const VulkanDevice &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) : 
    device{device}, size{size}, usage{usage}, properties{properties}
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device.getHandle(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.getHandle(), buffer, &memRequirements);

    VkMemoryAllocateFlagsInfo allocFlagsInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
    if (hasFlag(usage, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
        allocFlagsInfo.flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    }
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.getGPU().findMemoryType(memRequirements.memoryTypeBits, properties);
    allocInfo.pNext = &allocFlagsInfo;

    if (vkAllocateMemory(device.getHandle(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device.getHandle(), buffer, memory, 0);
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept:
    device{other.device},
    size{other.size},
    usage{other.usage},
    properties{other.properties},
    buffer{other.buffer},
    memory{other.memory},
    mappedData{other.mappedData}
{
    other.buffer = VK_NULL_HANDLE;
    other.memory = VK_NULL_HANDLE;
}

VulkanBuffer::~VulkanBuffer()
{
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device.getHandle(), buffer, nullptr);
        vkFreeMemory(device.getHandle(), memory, nullptr);
    }
}

void VulkanBuffer::map(void*& target)
{
    vkMapMemory(device.getHandle(), memory, this->offset, this->size, 0, &target);
}

void VulkanBuffer::unmap()
{
    vkUnmapMemory(device.getHandle(), memory);
}

void VulkanBuffer::update(const void *data, VkDeviceSize size, VkDeviceSize offset)
{
    map(mappedData);
    memcpy(reinterpret_cast<uint8_t *>(mappedData) + offset, data, static_cast<size_t>(size));
    unmap();
}

VkDescriptorBufferInfo VulkanBuffer::getBufferInfo() const
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = size;

    return bufferInfo;
}

VkBuffer VulkanBuffer::getHandle() const { return buffer; }