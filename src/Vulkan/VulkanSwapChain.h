#pragma once

#include <algorithm>
#include <vector>
#include <initializer_list>

#include "VulkanCommon.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"

class VulkanSwapChain {
public:
    VulkanSwapChain(const VulkanDevice& device, VkSurfaceKHR surface, VkExtent2D extent);

    ~VulkanSwapChain();

    VkResult acquireNextImage(uint32_t& index, VkSemaphore semaphore, VkFence fence) const;

    VkSurfaceKHR getSurface() const;
    VkSwapchainKHR getHandle() const;
    const std::vector<VkImage>& getImages() const;
    VkExtent2D getExtent() const;
    VkExtent3D getExtent3D() const;
    VkFormat getImageFormat() const;
    VkImageUsageFlags getImageUsage() const;

private:
    const VulkanDevice& device;
    VkSurfaceKHR surface;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkImageUsageFlags swapChainImageUsage;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkExtent2D& requiredExtent, const VkSurfaceCapabilitiesKHR& capabilities);
};