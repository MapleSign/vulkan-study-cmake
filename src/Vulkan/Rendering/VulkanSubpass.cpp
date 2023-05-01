#include "VulkanSubpass.h"

#include "VulkanResource.h"

VulkanSubpass::VulkanSubpass(const VulkanDevice& device, VulkanResourceManager& resManager,
    VulkanShaderModule&& vertShader, VulkanShaderModule&& fragShader) :
    device{ device }, resManager{ resManager },
    vertShader{ std::move(vertShader) }, fragShader{ std::move(fragShader) }
{
}

VulkanSubpass::~VulkanSubpass()
{
}
