#pragma once

#include <memory>

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "Rendering/VulkanRenderContext.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanPipelineLayout.h"
#include "VulkanTexture.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDescriptorSet.h"
#include "Rendering/VulkanRenderPipeline.h"
#include "Rendering/VulkanResource.h"
#include "Rendering/VulkanRayTracingBuilder.h"
#include "VulkanRayTracingPipeline.h"

class VulkanInstance;
class VulkanDevice;
class VulkanPhysicalDevice;
class VulkanSwapChain;
class VulkanImage;
class VulkanImageView;