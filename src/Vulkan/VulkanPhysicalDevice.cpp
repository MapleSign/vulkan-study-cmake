#include <optional>
#include <vector>

#include "VulkanCommon.h"
#include "VulkanPhysicalDevice.h"


bool QueueFamilyIndices::isComplete() {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

VulkanPhysicalDevice::VulkanPhysicalDevice(VkPhysicalDevice device) {
	physicalDevice = device;

	vkGetPhysicalDeviceProperties(device, &properties);
	vkGetPhysicalDeviceFeatures(device, &features);

	// raytracing properties
	rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	properties2.pNext = &rtProperties;
	vkGetPhysicalDeviceProperties2(device, &properties2);

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	queueFamilies.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	extensions.resize(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
}

VulkanPhysicalDevice::~VulkanPhysicalDevice() {

}

VkPhysicalDevice VulkanPhysicalDevice::getHandle() const { return physicalDevice; }

const VkPhysicalDeviceProperties& VulkanPhysicalDevice::getProperties() const { return properties; }
const VkPhysicalDeviceFeatures VulkanPhysicalDevice::getFeatures() const { return features; }
const std::vector<VkQueueFamilyProperties>& VulkanPhysicalDevice::getQueueFamalies() const { return queueFamilies; }
const std::vector<VkExtensionProperties>& VulkanPhysicalDevice::getExtensions() const { return extensions; }

VkBool32 VulkanPhysicalDevice::isPresentSupported(VkSurfaceKHR surface, uint32_t queueFamilyIndex) const {
	VkBool32 presentSupported{ VK_FALSE };

	if (surface != VK_NULL_HANDLE) {
		VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &presentSupported));
	}

	return presentSupported;
}

bool VulkanPhysicalDevice::isSwapChainSupported(VkSurfaceKHR surface) const {
	const auto&& details = querySwapChainSupport(surface);

	return !details.formats.empty() && !details.presentModes.empty();
}

QueueFamilyIndices VulkanPhysicalDevice::findQueueFamilies(VkSurfaceKHR surface) const {
	QueueFamilyIndices indices;

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

SwapChainSupportDetails VulkanPhysicalDevice::querySwapChainSupport(VkSurfaceKHR surface) const {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

int VulkanPhysicalDevice::getScore() const {
	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += properties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!features.geometryShader) {
		return 0;
	}

	return score;
}

uint32_t VulkanPhysicalDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

size_t VulkanPhysicalDevice::pad_uniform_buffer_size(size_t originalSize) const
{
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}
