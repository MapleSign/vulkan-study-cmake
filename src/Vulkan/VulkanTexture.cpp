#include <stb_image.h>

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "VulkanCommandPool.h"
#include "VulkanBuffer.h"
#include "VulkanQueue.h"
#include "VulkanTexture.h"

VulkanTexture::VulkanTexture(
    const VulkanDevice& device, const void* data, size_t size, VkExtent3D extent, VkFormat format, VkSampler sampler,
    const VulkanCommandPool& commandPool, const VulkanQueue& queue) :
    device{ device }, sampler{ sampler }, mipLevels{ 1 }, arrayLayers{ 1 }
{
    image = std::make_unique<VulkanImage>(
        device, extent,
        format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        0,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mipLevels
    );

    imageView = std::make_unique<VulkanImageView>(*image);

    if (data != nullptr) {
        return;
    }

    VulkanBuffer stagingBuffer{ device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
    stagingBuffer.update(data, size);

    commandPool.transitionImageLayout(*image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, queue);
    commandPool.copyBufferToImage(stagingBuffer, *image, queue);
    //commandPool.transitionImageLayout(*image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, queue);
}

VulkanTexture::VulkanTexture(
    const VulkanDevice& device, const char* filename, VkSampler sampler,
    const VulkanCommandPool& commandPool, const VulkanQueue& queue) :
    device{ device }, sampler{ sampler }, mipLevels{ 1 }, arrayLayers{ 1 }
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    mipLevels = toU32(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;


    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VulkanBuffer stagingBuffer{ device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
    stagingBuffer.update(pixels, imageSize);

    stbi_image_free(pixels);

    image = std::make_unique<VulkanImage>(
        device, VkExtent3D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 },
        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        0, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mipLevels
    );

    commandPool.transitionImageLayout(*image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, queue);
    commandPool.copyBufferToImage(stagingBuffer, *image, queue);
    //commandPool.transitionImageLayout(*image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, queue);

    commandPool.generateMipmaps(*image, queue);

    imageView = std::make_unique<VulkanImageView>(*image);
}

VulkanTexture::VulkanTexture(
    const VulkanDevice& device, const std::vector<std::string>& filenames, VkSampler sampler,
    const VulkanCommandPool& commandPool, const VulkanQueue& queue) :
    device{ device }, sampler{ sampler }, mipLevels{ 1 }
{
    arrayLayers = toU32(filenames.size());
    int imageWidth, imageHeight;
    VkDeviceSize imageSize;

    std::unique_ptr<VulkanBuffer> stagingBuffer{};
    std::vector<VkDeviceSize> offsets{};
    VkDeviceSize offset = 0;
    for (int i = 0; i < arrayLayers; ++i) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(filenames[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        if (i == 0) {
            imageWidth = texWidth;
            imageHeight = texHeight;
            imageSize = texWidth * texHeight * 4;
            mipLevels = toU32(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

            stagingBuffer = std::make_unique<VulkanBuffer>(
                device, imageSize * arrayLayers, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }

        if (imageWidth != texWidth || imageHeight != texWidth) {
            throw std::runtime_error("Image with multi layers must have same size!");
        }

        stagingBuffer->update(pixels, imageSize, offset);
        stbi_image_free(pixels);

        offsets.push_back(offset);
        offset += imageSize;
    }

    image = std::make_unique<VulkanImage>(
        device, VkExtent3D{ static_cast<uint32_t>(imageWidth), static_cast<uint32_t>(imageHeight), 1 },
        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mipLevels, arrayLayers
        );

    commandPool.transitionImageLayout(*image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, queue);
    commandPool.copyBufferToImage(*stagingBuffer, *image, queue);

    commandPool.generateMipmaps(*image, queue);

    imageView = std::make_unique<VulkanImageView>(*image);
}

VulkanTexture::~VulkanTexture() {
    imageView.reset();
    image.reset();
}

VkDescriptorImageInfo VulkanTexture::getImageInfo() const {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView->getHandle();
    imageInfo.sampler = sampler;

    return imageInfo;
}