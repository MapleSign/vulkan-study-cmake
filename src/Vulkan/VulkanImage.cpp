#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"

VulkanImage::VulkanImage(const VulkanDevice& device, const VkExtent3D& extent, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkImageCreateFlags flags, VkMemoryPropertyFlags properties,
    uint32_t mipLevels, uint32_t arrayLayers) :
    extent{ extent }, device{ device }, format{ format }, sampleCount{ VK_SAMPLE_COUNT_1_BIT },
    usage{ usage }, flags{ flags }, mipLevels{ mipLevels }, arrayLayers{ arrayLayers }
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = extent;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.flags = flags;
    imageInfo.samples = sampleCount;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device.getHandle(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.getHandle(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.getGPU().findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device.getHandle(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device.getHandle(), image, imageMemory, 0);
}

VulkanImage::VulkanImage(const VulkanDevice& device, VkImage handle, const VkExtent3D& extent, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels) :
    device{ device }, image{ handle }, extent{ extent }, format{ format }, usage{ usage }, sampleCount{ VK_SAMPLE_COUNT_1_BIT }, mipLevels{ mipLevels }
{
}

VulkanImage::VulkanImage(VulkanImage&& other) noexcept :
    device{ other.device },
    image{ other.image },
    imageMemory{ other.imageMemory },
    extent{ other.extent },
    format{ other.format },
    usage{ other.usage },
    flags{ other.flags },
    sampleCount{ other.sampleCount },
    mipLevels{ other.mipLevels },
    arrayLayers{ other.arrayLayers }
{
    other.image = VK_NULL_HANDLE;
    other.imageMemory = VK_NULL_HANDLE;
}

VulkanImage::~VulkanImage() {
    if (image != VK_NULL_HANDLE && imageMemory != VK_NULL_HANDLE) {
        vkDestroyImage(device.getHandle(), image, nullptr);
        vkFreeMemory(device.getHandle(), imageMemory, nullptr);
    }
}

VkImage VulkanImage::getHandle() const { return image; }
VkDeviceMemory VulkanImage::getMemory() const { return imageMemory; }

const VkExtent3D &VulkanImage::getExtent() const { return extent; }
VkFormat VulkanImage::getFormat() const { return format; }
VkSampleCountFlagBits VulkanImage::getSampleCount() const { return sampleCount; }
VkImageUsageFlags VulkanImage::getUsage() const { return usage; }

VkImageCreateFlags VulkanImage::getFlags() const
{
    return flags;
}

uint32_t VulkanImage::getMipLevels() const
{
    return mipLevels;
}

uint32_t VulkanImage::getArrayLayers() const
{
    return arrayLayers;
}

const VulkanDevice &VulkanImage::getDevice() const { return device; }
