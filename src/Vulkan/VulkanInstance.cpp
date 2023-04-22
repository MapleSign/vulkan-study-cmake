#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "VulkanCommon.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanInstance.h"

VulkanInstance::VulkanInstance(std::vector<const char*> requiredExtensions, std::vector<const char*> requiredValidationLayers) {
    this->requiredExtensions = std::vector<std::string>(requiredExtensions.begin(), requiredExtensions.end());

    if (!checkValidationLayerSupport(requiredValidationLayers)) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (!requiredValidationLayers.empty()) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
        createInfo.ppEnabledLayerNames = requiredValidationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
    volkLoadInstance(instance);

    queryGPUs();

    setupDebugMessenger();
}

VulkanInstance::~VulkanInstance() {
    if (instance != VK_NULL_HANDLE) {
        destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
}

VkInstance VulkanInstance::getHandle() const { return instance; }

VulkanPhysicalDevice& VulkanInstance::getSuitableGPU(VkSurfaceKHR surface, const std::vector<const char *> deviceExtentions) const {
    std::multimap<int, decltype(gpus)::const_iterator> candidates;

    for (auto it = gpus.begin(); it != gpus.end(); ++it) {
        if (isDeviceSuitable(surface, **it, deviceExtentions)) {
            int score = (*it)->getScore();
            candidates.insert(std::make_pair(score, it));
        }
    }

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0) {
        return **candidates.rbegin()->second;
    }
    else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void VulkanInstance::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VulkanInstance::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanInstance::queryGPUs() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (auto &&device : devices) {
        gpus.emplace_back(std::make_unique<VulkanPhysicalDevice>(device));
    }
}

bool VulkanInstance::checkValidationLayerSupport(std::vector<const char *> requiredValidationLayers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : requiredValidationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void VulkanInstance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

bool VulkanInstance::isDeviceSuitable(VkSurfaceKHR surface, VulkanPhysicalDevice& device, const std::vector<const char*> deviceExtentions) const {
    bool queueFamiliseSupported = device.findQueueFamilies(surface).isComplete();

    bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtentions);

    bool swapChainAdequate = device.isSwapChainSupported(surface);

    return queueFamiliseSupported && extensionsSupported && swapChainAdequate;
}

bool VulkanInstance::checkDeviceExtensionSupport(VulkanPhysicalDevice& device, const std::vector<const char*> deviceExtentions) const {
    std::set<std::string> requiredExtensions(deviceExtentions.begin(), deviceExtentions.end());

    for (const auto& extension : device.getExtensions()) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}