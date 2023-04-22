#pragma once

#include <array>

#include "VulkanCommon.h"
#include "VulkanDevice.h"

class VulkanRenderPass
{
public:
    VulkanRenderPass(const VulkanDevice& device, VkFormat swapChainImageFormat);

    ~VulkanRenderPass();

    VkRenderPass getHandle() const;

private:
    VkRenderPass renderPass;

    const VulkanDevice& device;
};
