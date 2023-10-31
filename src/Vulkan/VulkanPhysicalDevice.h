#pragma once

#include <optional>
#include <vector>

#include "VulkanCommon.h"

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete();
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanPhysicalDevice {
public:
	VulkanPhysicalDevice(VkPhysicalDevice device);

	~VulkanPhysicalDevice();
	
	VkPhysicalDevice getHandle() const;

	const VkPhysicalDeviceProperties& getProperties() const;
	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& getRTProperties() const;
	const VkPhysicalDeviceFeatures getFeatures() const;
	const std::vector<VkQueueFamilyProperties>& getQueueFamalies() const;
	const std::vector<VkExtensionProperties>& getExtensions() const;

	VkBool32 isPresentSupported(VkSurfaceKHR surface, uint32_t queueFamilyIndex) const;

	bool isSwapChainSupported(VkSurfaceKHR surface) const;

	QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface) const;

	SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface) const;

	int getScore() const;

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

	size_t pad_uniform_buffer_size(size_t originalSize) const;
private:
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VkPhysicalDeviceProperties properties{};
	VkPhysicalDeviceFeatures features{};

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
	VkPhysicalDeviceProperties2 properties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };

	VkPhysicalDeviceShaderClockFeaturesKHR clockFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR };
	VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
	VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
	VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };


	std::vector<VkQueueFamilyProperties> queueFamilies;
	std::vector<VkExtensionProperties> extensions;
};