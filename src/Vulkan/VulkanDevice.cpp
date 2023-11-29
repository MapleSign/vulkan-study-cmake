#include "VulkanDevice.h"

#include "VulkanQueue.h"
#include "VulkanCommandPool.h"

#include <cstring>

VulkanDevice::VulkanDevice(const VulkanPhysicalDevice &physicalDevice, VkSurfaceKHR surface, 
    const std::vector<const char *> &requiredExtentions, const std::vector<const char *> &requiredLayers) :
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
    deviceFeatures.features.imageCubeArray = VK_TRUE;

    VkPhysicalDeviceShaderClockFeaturesKHR clockFreature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR };
    clockFreature.shaderDeviceClock = VK_TRUE;
    clockFreature.shaderSubgroupClock = VK_TRUE;

    deviceFeatures.pNext = &clockFreature;

    VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = VK_TRUE;
    features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    features12.descriptorBindingPartiallyBound = VK_TRUE;
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.descriptorIndexing = VK_TRUE;
    features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

    clockFreature.pNext = &features12;


    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    if (std::find_if(requiredExtentions.begin(), requiredExtentions.end(), [](const char* ext) { return !strcmp(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, ext); }) != requiredExtentions.end()) {
        rtPipelineFeatures.rayTracingPipeline = VK_TRUE;
        features12.pNext = &rtPipelineFeatures;

        asFeatures.accelerationStructure = VK_TRUE;
        rtPipelineFeatures.pNext = &asFeatures;
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    //createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.pNext = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtentions.size());
    createInfo.ppEnabledExtensionNames = requiredExtentions.data();

    if (!requiredLayers.empty()) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
        createInfo.ppEnabledLayerNames = requiredLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    auto result = vkCreateDevice(physicalDevice.getHandle(), &createInfo, nullptr, &device);
    if (result != VK_SUCCESS) {
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
