#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <optional>
#include <map>
#include <set>
#include <vector>

#include "VulkanCommon.h"
#include "VulkanPhysicalDevice.h"

class VulkanQueue;
class VulkanCommandPool;

class VulkanDevice {
public:
    VulkanDevice(const VulkanPhysicalDevice& physicalDevice, VkSurfaceKHR surface,
        const std::vector<const char*>& requiredExtentions, const std::vector<const char*>& requiredLayers);

    ~VulkanDevice();

    void waitIdle() const;

    VkDevice getHandle() const;

    const VulkanPhysicalDevice& getGPU() const;
    VulkanCommandPool& getCommandPool() const;
    VulkanQueue& getGraphicsQueue() const;
    VulkanQueue& getPresentQueue() const;

private:
    const VulkanPhysicalDevice& physicalDevice;
    VkDevice device;

    std::unique_ptr<VulkanQueue> graphicsQueue;
    std::unique_ptr<VulkanQueue> presentQueue;

    std::unique_ptr<VulkanCommandPool> commandPool;
};