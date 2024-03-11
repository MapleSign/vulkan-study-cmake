#include "SSAOSubpass.h"

#include <random>

SSAOSubpass::SSAOSubpass(const VulkanDevice& device, VulkanResourceManager& resManager, VkExtent2D extent, 
	const std::vector<VulkanShaderResource> shaderRes, const VulkanRenderPass& renderPass, uint32_t subpass):
	VulkanSubpass(device, resManager, extent, shaderRes, renderPass, subpass)
{
	auto vertShader = resManager.createShaderModule("shaders/spv/passthrough.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");

	auto fragShader = resManager.createShaderModule("shaders/spv/ssao.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	fragShader.addShaderResourceUniform(ShaderResourceType::Uniform, 0, 0);
	fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 1);
	fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 2);
	fragShader.addShaderResourceUniform(ShaderResourceType::Sampler, 0, 3);

	renderPipeline = std::make_unique<VulkanRenderPipeline>(device, resManager, std::move(vertShader), std::move(fragShader));
	renderPipeline->prepare();
	auto& state = renderPipeline->getPipelineState();
	state.subpass = subpass;
	state.cullMode = VK_CULL_MODE_NONE;
	state.depthStencilState.depth_test_enable = VK_FALSE;
	state.depthStencilState.depth_write_enable = VK_FALSE;
	state.vertexBindingDescriptions = {};
	state.vertexAttributeDescriptions = {};
	renderPipeline->recreatePipeline(extent, renderPass);

	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
	std::default_random_engine generator;

	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
		);
		sample = glm::normalize(sample); // random direction
		sample *= randomFloats(generator); // random distance(position)

		// sample more close to center
		float scale = (float)i / 64.0;
		scale = glm::mix(0.1f, 1.0f, scale * scale);
		sample *= scale;

		ssaoData.samples[i] = glm::vec4(sample, 0.0);
	}

	std::vector<glm::vec2> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec2 noise(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0);
		ssaoNoise.push_back(noise);
	}

	noiseImage = std::make_unique<VulkanImage>(device, VkExtent3D{ 4, 4, 1 }, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	noiseImageView = std::make_unique<VulkanImageView>(*noiseImage);

	VulkanBuffer stagingBuffer{ device, ssaoNoise.size() * sizeof(glm::vec2), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
	stagingBuffer.update(ssaoNoise.data(), ssaoNoise.size() * sizeof(glm::vec2));

	device.getCommandPool().transitionImageLayout(*noiseImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, device.getGraphicsQueue());
	device.getCommandPool().copyBufferToImage(stagingBuffer, *noiseImage, device.getGraphicsQueue());
	device.getCommandPool().transitionImageLayout(*noiseImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, device.getGraphicsQueue());
}

SSAOSubpass::~SSAOSubpass()
{
}

void SSAOSubpass::prepare(const std::vector<const VulkanImageView*>& gBuffers)
{
	ssaoSceneData = resManager.requireSceneData(*renderPipeline->getDescriptorSetLayouts()[0], 1,
		{
			{0, {device.getGPU().pad_uniform_buffer_size(sizeof(SSAOData)), 1}}
		}
	);

	VkSamplerCreateInfo samplerInfo = resManager.getDefaultSamplerCreateInfo();

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VkSampler gBufferSampler = resManager.createSampler(&samplerInfo);
	ssaoSceneData.descriptorSets[0]->addWrite(1,
		VkDescriptorImageInfo{ gBufferSampler, gBuffers[GBufferType::Position]->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	);
	ssaoSceneData.descriptorSets[0]->addWrite(2,
		VkDescriptorImageInfo{ gBufferSampler, gBuffers[GBufferType::Normal]->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	);

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSampler noiseSampler = resManager.createSampler(&samplerInfo);
	ssaoSceneData.descriptorSets[0]->addWrite(3,
		VkDescriptorImageInfo{ noiseSampler, noiseImageView->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	);

	ssaoSceneData.update();
}

void SSAOSubpass::update(float deltaTime, const Scene* scene)
{
	const auto& camera = scene->getActiveCamera();

	ssaoData.view = camera->calcLookAt();
	ssaoData.projection = glm::perspective(glm::radians(camera->zoom), (float)extent.width / (float)extent.height, camera->zNear, camera->zFar);
	ssaoData.projection[1][1] *= -1;

	ssaoData.windowSize = { extent.width, extent.height };

	ssaoSceneData.updateData(0, 0, &ssaoData, sizeof(ssaoData));
}

void SSAOSubpass::draw(VulkanCommandBuffer& cmdBuf, const std::vector<VulkanDescriptorSet*>& globalSets)
{
	cmdBuf.bindPipeline(renderPipeline->getGraphicsPipeline());

	std::vector<VkDescriptorSet> descSets = { ssaoSceneData.descriptorSets[0]->getHandle() };
	vkCmdBindDescriptorSets(cmdBuf.getHandle(),
		renderPipeline->getGraphicsPipeline().getBindPoint(),
		renderPipeline->getPipelineLayout().getHandle(),
		0, descSets.size(), descSets.data(), 0, nullptr);

	vkCmdDraw(cmdBuf.getHandle(), 3, 1, 0, 0);
}
