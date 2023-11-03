#pragma once

#include <array>

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanRenderTarget.h"

struct LoadStoreInfo
{
    VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;

    VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_STORE;
};

struct SubpassInfo
{
    std::vector<uint32_t> output{};
    std::vector<uint32_t> input{};
    std::vector<VkSubpassDependency> dependencies{};
};

class VulkanRenderPass
{
public:
    VulkanRenderPass(const VulkanDevice& device, 
        const std::vector<VulkanAttatchment>& attachments, 
        const std::vector<LoadStoreInfo>& loadStoreInfos, 
        const std::vector<SubpassInfo>& subpasses = {});

    ~VulkanRenderPass();

    VkRenderPass getHandle() const;

private:
    VkRenderPass renderPass;

    const VulkanDevice& device;
};
