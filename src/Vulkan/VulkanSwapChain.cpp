#include <algorithm>
#include <vector>
#include <initializer_list>

#include "VulkanCommon.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

VulkanSwapChain::VulkanSwapChain(const VulkanDevice &device, VkSurfaceKHR surface, VkExtent2D extent):
    device{device}, swapChainImageUsage{ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }
{
    SwapChainSupportDetails swapChainSupport = device.getGPU().querySwapChainSupport(surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    extent = chooseSwapExtent(extent, swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = swapChainImageUsage;

    QueueFamilyIndices indices = device.getGPU().findQueueFamilies(surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device.getHandle(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device.getHandle(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device.getHandle(), swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

VulkanSwapChain::~VulkanSwapChain() {
    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device.getHandle(), swapChain, nullptr);
    }
}

VkResult VulkanSwapChain::acquireNextImage(uint32_t &index, VkSemaphore semaphore, VkFence fence) const {
    return vkAcquireNextImageKHR(device.getHandle(), swapChain, UINT64_MAX, semaphore, fence, &index);
}

VkSurfaceKHR VulkanSwapChain::getSurface() const { return surface; }
VkSwapchainKHR VulkanSwapChain::getHandle() const { return swapChain; }
const std::vector<VkImage>& VulkanSwapChain::getImages() const { return swapChainImages; }
VkExtent2D VulkanSwapChain::getExtent() const { return swapChainExtent; }
VkExtent3D VulkanSwapChain::getExtent3D() const { return convert2Dto3D(swapChainExtent); }
VkFormat VulkanSwapChain::getImageFormat() const { return swapChainImageFormat; }
VkImageUsageFlags VulkanSwapChain::getImageUsage() const { return swapChainImageUsage; }

VkSurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapChain::chooseSwapExtent(const VkExtent2D &requiredExtent, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        int width = requiredExtent.width;
        int height = requiredExtent.height;

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
