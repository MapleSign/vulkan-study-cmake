#include <array>

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"

VulkanRenderPass::VulkanRenderPass(const VulkanDevice& device,
    const std::vector<VulkanAttatchment>& attachments,
    const std::vector<LoadStoreInfo>& loadStoreInfos) :
    device{ device }
{
    std::vector<VkAttachmentDescription> attachDescs{ attachments.size() };
    for (size_t i = 0; i < attachments.size(); ++i) {
        VkAttachmentDescription attachDesc{};
        attachDesc.format = attachments[i].format;
        attachDesc.samples = attachments[i].sample;

        attachDesc.loadOp = loadStoreInfos[i].load_op;
        attachDesc.storeOp = loadStoreInfos[i].store_op;

        attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        attachDesc.initialLayout = attachments[i].initialLayout;
        attachDesc.finalLayout = attachments[i].finalLayout == VK_IMAGE_LAYOUT_UNDEFINED ?
            isDepthStencilFormat(attachments[i].format) ? 
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            : attachments[i].finalLayout;

        attachDescs[i] = attachDesc;
    }

    std::vector<VkAttachmentReference> colorRefs;
    VkAttachmentReference depthRef{-1};
    for (size_t i = 0; i < attachDescs.size(); ++i) {
        VkAttachmentReference ref{};
        ref.attachment = toU32(i);

        if (isDepthStencilFormat(attachDescs[i].format)) {
            ref.layout = attachDescs[i].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED ?
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : attachDescs[i].initialLayout;
            depthRef = ref;
        }
        else {
            ref.layout = attachDescs[i].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED ?
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : attachDescs[i].initialLayout;
            colorRefs.push_back(ref);
        }
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = toU32(colorRefs.size());
    subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
    subpass.pDepthStencilAttachment = depthRef.attachment == -1 ? nullptr : &depthRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = toU32(attachDescs.size());
    renderPassInfo.pAttachments = attachDescs.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.getHandle(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

VulkanRenderPass::~VulkanRenderPass() {
    if (renderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(device.getHandle(), renderPass, nullptr);
}

VkRenderPass VulkanRenderPass::getHandle() const { return renderPass; }
