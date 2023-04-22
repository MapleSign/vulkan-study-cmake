#pragma once

#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "../Vulkan/VulkanInstance.h"
#include "../Vulkan/VulkanDevice.h"
#include "../Vulkan/VulkanRenderPass.h"

class GlfwWindow;

class GUI
{
public:
	GUI(const VulkanInstance& instance, const GlfwWindow& window, const VulkanDevice& device, const VulkanRenderPass& renderPass, 
		uint32_t minImageCount = 2, uint32_t imageCount = 2, VkSampleCountFlagBits MSAASamples = VK_SAMPLE_COUNT_1_BIT);
	~GUI();

	void newFrame() const;
	void render() const;
	void renderDrawData(const VkCommandBuffer& cmd) const;

private:
	const VulkanDevice& device;

	VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;

	uint32_t minImageCount;
	uint32_t imageCount;
	VkSampleCountFlagBits MSAASamples;
};