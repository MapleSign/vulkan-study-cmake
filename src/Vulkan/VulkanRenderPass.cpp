#include <array>

#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"

VulkanRenderPass::VulkanRenderPass(const VulkanDevice& device,
    const std::vector<VulkanAttatchment>& attachments,
    const std::vector<LoadStoreInfo>& loadStoreInfos,
    const std::vector<SubpassInfo>& subpassInfos) :
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

    std::vector<VkAttachmentReference> refs;
    int depthRefIdx = -1;
    for (size_t i = 0; i < attachDescs.size(); ++i) {
        VkAttachmentReference ref{};
        ref.attachment = toU32(i);

        if (isDepthStencilFormat(attachDescs[i].format)) {
            ref.layout = attachDescs[i].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED ?
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : attachDescs[i].initialLayout;
            depthRefIdx = i;
        }
        else {
            ref.layout = attachDescs[i].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED ?
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : attachDescs[i].initialLayout;
        }
        refs.push_back(ref);
    }

    std::vector<std::vector<VkAttachmentReference>> colorRefsList{ subpassInfos.size() };
    std::vector<std::optional<VkAttachmentReference>> depthRefList{ subpassInfos.size() };
    std::vector<std::vector<VkAttachmentReference>> inputRefsList{ subpassInfos.size() };
    std::vector<VkSubpassDescription> subpasses{ subpassInfos.size() };
    if (subpasses.empty()) {
        colorRefsList.push_back(refs);
        auto& colorRefs = colorRefsList.back();

        if (depthRefIdx > -1) {
            depthRefList.push_back(refs[depthRefIdx]);
            colorRefs.erase(colorRefs.begin() + depthRefIdx);
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = toU32(colorRefs.size());
        subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
        subpass.pDepthStencilAttachment = !depthRefList.back().has_value() ? nullptr : &depthRefList.back().value();

        subpasses.push_back(subpass);
    }
    else {
        for (size_t i = 0; i < subpassInfos.size(); ++i) {
            for (const auto& attach : subpassInfos[i].input) {
                inputRefsList[i].push_back(refs[attach]);
                inputRefsList[i].back().layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }

            for (const auto& attach : subpassInfos[i].output) {
                if (attach != depthRefIdx)
                    colorRefsList[i].push_back(refs[attach]);
                else
                    depthRefList[i] = refs[attach];
            }

            subpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            subpasses[i].inputAttachmentCount = toU32(inputRefsList[i].size());
            subpasses[i].pInputAttachments = inputRefsList[i].empty() ? nullptr : inputRefsList[i].data();

            subpasses[i].colorAttachmentCount = toU32(colorRefsList[i].size());
            subpasses[i].pColorAttachments = colorRefsList[i].empty() ? nullptr : colorRefsList[i].data();
            subpasses[i].pDepthStencilAttachment = !depthRefList[i].has_value() ? nullptr : &depthRefList[i].value();
        }
    }

    std::vector<VkSubpassDependency> dependencies{};
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependencies.push_back(dependency);
    for (const auto& subpassInfo : subpassInfos) {
         dependencies.insert(dependencies.end(), subpassInfo.dependencies.begin(), subpassInfo.dependencies.end());
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = toU32(attachDescs.size());
    renderPassInfo.pAttachments = attachDescs.data();
    renderPassInfo.subpassCount = toU32(subpasses.size());
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = toU32(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device.getHandle(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

VulkanRenderPass::~VulkanRenderPass() {
    if (renderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(device.getHandle(), renderPass, nullptr);
}

VkRenderPass VulkanRenderPass::getHandle() const { return renderPass; }
