#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "VulkanCommon.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"

struct VulkanAttatchment
{
	VkFormat format{ VK_FORMAT_UNDEFINED };
	VkSampleCountFlagBits sample{ VK_SAMPLE_COUNT_1_BIT };
	VkImageUsageFlags usage{ VK_IMAGE_USAGE_SAMPLED_BIT };
	VkImageLayout initialLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkImageLayout finalLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

	VulkanAttatchment() = default;
	VulkanAttatchment(VkFormat format, VkSampleCountFlagBits sample, VkImageUsageFlags usage):
		format{format}, sample{sample}, usage{usage}
	{}
};

class VulkanRenderTarget
{
public:
	using  CreateFunc = std::function<std::unique_ptr<VulkanRenderTarget>(VulkanImage&&)>;
	static const CreateFunc DEFAULT_CREATE_FUNC;

	VulkanRenderTarget(std::vector<VulkanImage>&& images);
	VulkanRenderTarget(std::vector<VulkanImageView>&& imageViews);

	~VulkanRenderTarget();

	const std::vector<VulkanImage>& getImages() const;
	const std::vector<VulkanImageView>& getViews() const;
	VkExtent2D getExtent() const;
	uint32_t getLayers() const;
	const std::vector<VulkanAttatchment>& getAttatchments() const;

private:
	const VulkanDevice& device;

	std::vector<VulkanImage> images;
	std::vector<VulkanImageView> imageViews;
	std::vector<VulkanAttatchment> attatchments;

	VkExtent2D extent;
	uint32_t layers;

	std::vector<uint32_t> inputAttatchments = {};
	std::vector<uint32_t> outputAttatchments = { 0 };
};