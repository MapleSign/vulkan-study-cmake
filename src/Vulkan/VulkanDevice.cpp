#include "VulkanDevice.h"

#include "VulkanQueue.h"
#include "VulkanCommandPool.h"

VulkanDevice::VulkanDevice(const VulkanPhysicalDevice &physicalDevice, VkSurfaceKHR surface, 
    const std::vector<const char *> &deviceExtentions, const std::vector<const char *> &requiredLayers) :
    physicalDevice{physicalDevice}
{
    QueueFamilyIndices indices = physicalDevice.findQueueFamilies(surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures2 deviceFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    deviceFeatures.features.samplerAnisotropy = VK_TRUE;
    deviceFeatures.features.geometryShader = VK_TRUE;
    deviceFeatures.features.shaderInt64 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = VK_TRUE;
    features12.runtimeDescriptorArray = VK_TRUE;

    deviceFeatures.pNext = &features12;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    //createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.pNext = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtentions.size());
    createInfo.ppEnabledExtensionNames = deviceExtentions.data();

    if (!requiredLayers.empty()) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
        createInfo.ppEnabledLayerNames = requiredLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice.getHandle(), &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    graphicsQueue = std::make_unique<VulkanQueue>(*this, indices.graphicsFamily.value());
    presentQueue = std::make_unique<VulkanQueue>(*this, indices.presentFamily.value());

    commandPool = std::make_unique<VulkanCommandPool>(*this, indices.graphicsFamily.value());
}

VulkanDevice::~VulkanDevice() {
    commandPool = nullptr;

    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
    }
}

void VulkanDevice::waitIdle() const {
    vkDeviceWaitIdle(device);
}

VkDevice VulkanDevice::getHandle() const { return device; }

const VulkanPhysicalDevice& VulkanDevice::getGPU() const { return physicalDevice; }
VulkanCommandPool& VulkanDevice::getCommandPool() const { return *commandPool; }
VulkanQueue& VulkanDevice::getGraphicsQueue() const { return *graphicsQueue; }
VulkanQueue& VulkanDevice::getPresentQueue() const { return *presentQueue; }
