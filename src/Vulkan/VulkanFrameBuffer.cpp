#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanRenderTarget.h"
#include "VulkanRenderPass.h"
#include "VulkanFrameBuffer.h"

VulkanFramebuffer::VulkanFramebuffer(const VulkanDevice& device, const VulkanRenderTarget& renderTarget, const VulkanRenderPass& renderPass):
    device{device}
{
    std::vector<VkImageView> attachments;
    for (const auto& view : renderTarget.getViews()) {
        attachments.emplace_back(view.getHandle());
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass.getHandle();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = renderTarget.getExtent().width;
    framebufferInfo.height = renderTarget.getExtent().height;
    framebufferInfo.layers = renderTarget.getLayers();

    if (vkCreateFramebuffer(device.getHandle(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

VulkanFramebuffer::~VulkanFramebuffer() {
    if (framebuffer != VK_NULL_HANDLE)
        vkDestroyFramebuffer(device.getHandle(), framebuffer, nullptr);
}

VkFramebuffer VulkanFramebuffer::getHandle() const { return framebuffer; }
