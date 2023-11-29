#pragma once

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <memory>

#include "VulkanCommon.h"
#include "VulkanPhysicalDevice.h"

class VulkanInstance {
public:
    VulkanInstance(std::vector<const char*> requiredExtensions, std::vector<const char*> requiredValidationLayers);

    ~VulkanInstance();

    VkInstance getHandle() const;

    VulkanPhysicalDevice& getSuitableGPU(VkSurfaceKHR surface, const std::vector<const char*> deviceExtentions) const;

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    std::vector<std::string> requiredExtensions;
    std::vector<std::unique_ptr<VulkanPhysicalDevice>> gpus;

    void setupDebugMessenger();

    void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    void queryGPUs();

    bool checkValidationLayerSupport(std::vector<const char*> requiredValidationLayers);

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    bool isDeviceSuitable(VkSurfaceKHR surface, VulkanPhysicalDevice& device, const std::vector<const char*> deviceExtentions) const;

    bool checkDeviceExtensionSupport(VulkanPhysicalDevice& device, const std::vector<const char*> deviceExtentions) const;
};