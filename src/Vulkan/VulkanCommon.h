#pragma once

#include <glm/glm.hpp>

#include <Volk/volk.h>

#include <stdexcept>
#include <vector>
#include <fstream>

#define VK_CHECK(exp) if (exp != VK_SUCCESS) { throw std::runtime_error("failed on" #exp); }

template<typename T>
constexpr uint32_t toU32(T a);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, 
    const VkAllocationCallbacks * pAllocator, VkDebugUtilsMessengerEXT * pDebugMessenger);

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>&candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

bool isDepthOnlyFormat(VkFormat format);

bool isDepthStencilFormat(VkFormat format);

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

std::vector<char> readFile(const std::string & filename);

VkExtent3D convert2Dto3D(const VkExtent2D & extent);

bool hasStencilComponent(VkFormat format);

template<typename T>
constexpr uint32_t toU32(T a)
{
    return static_cast<uint32_t>(a);
}

VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer);

constexpr bool hasFlag(VkFlags item, VkFlags flag)
{
    return (item & flag) == flag;
}

VkTransformMatrixKHR toTransformMatrixKHR(glm::mat4 matrix);