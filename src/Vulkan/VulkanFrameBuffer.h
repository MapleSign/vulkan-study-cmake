#pragma once

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanRenderTarget.h"
#include "VulkanRenderPass.h"

class VulkanFramebuffer
{
public:
	VulkanFramebuffer(const VulkanDevice& device, const VulkanRenderTarget& renderTarget, const VulkanRenderPass& renderPass);

	~VulkanFramebuffer();

	VkFramebuffer getHandle() const;

private:
    VkFramebuffer framebuffer;

	const VulkanDevice& device;
};