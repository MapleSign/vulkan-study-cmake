#include "GlobalSubpass.h"

GlobalSubpass::GlobalSubpass(const VulkanDevice& device, VulkanResourceManager& resManager,
    VulkanShaderModule&& vertShader, VulkanShaderModule&& fragShader,
    const VulkanDescriptorSetLayout* globalDescSetLayout) :
    VulkanSubpass(device, resManager, std::move(vertShader), std::move(fragShader)),
    globalDescSetLayout{ globalDescSetLayout }
{
}

GlobalSubpass::~GlobalSubpass()
{
}

void GlobalSubpass::prepare()
{

}
