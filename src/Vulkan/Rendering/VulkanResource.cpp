#include "VulkanResource.h"

#include <algorithm>

VulkanResourceManager::VulkanResourceManager(const VulkanDevice& device, VulkanCommandPool& commandPool):
    device{device}, commandPool{commandPool}
{
    std::vector<VkDescriptorPoolSize> poolSizes{
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    requireDescriptorPool(poolSizes, 1000);
}

VulkanResourceManager::~VulkanResourceManager()
{
    descriptorSetSet.clear();

    descriptorPools.clear();

    bufferSet.clear();

    textureMap.clear();

    for (const auto& sampler : samplerSet)
        vkDestroySampler(device.getHandle(), sampler, nullptr);
}

RenderMeshID VulkanResourceManager::requireRenderMesh(
    const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const GltfMaterial& mat, 
    VulkanPipeline* pipeline, const VulkanDescriptorSetLayout& descSetLayout, uint32_t threadCount,
    const std::map<uint32_t, std::pair<VkDeviceSize, size_t>>& bufferSizeInfos, 
    const BindingMap<RenderTexture>& textureInfosMap)
{
    RenderMesh mesh{};
    mesh.pipeline = pipeline;
    mesh.descSetLayout = &descSetLayout;

    VkBufferUsageFlags flag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkBufferUsageFlags rayTracingFlags = // used also for building acceleration structures 
        flag | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    auto& vertexBuffer = requireBufferWithData(vertices, 
        rayTracingFlags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    auto& indexBuffer = requireBufferWithData(indices, 
        rayTracingFlags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    auto& matBuffer = requireBufferWithData(&mat, sizeof(mat),
        flag | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    mesh.vertexBuffer = vertexBuffer.getBufferInfo();
    mesh.vertexNum = toU32(vertices.size());

    mesh.indexBuffer = indexBuffer.getBufferInfo();
    mesh.indexNum = toU32(indices.size());
    mesh.indexType = VK_INDEX_TYPE_UINT32;

    mesh.matBuffer = matBuffer.getBufferInfo();

    VkDeviceSize uniformBufferSize = 0;
    for (const auto& [binding, bufferSizeInfo] : bufferSizeInfos) {
        uniformBufferSize += bufferSizeInfo.first * bufferSizeInfo.second;
    }

    BindingMap<VkDescriptorBufferInfo> bufferInfos{};
    for (uint32_t i = 0; i < threadCount; ++i) {
        mesh.uniformBuffers.emplace_back(
            &requireBuffer(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        );

        VkDeviceSize offset = 0;
        for (const auto& [bindingIndex, bufferSizeInfo] : bufferSizeInfos)
        {
            bufferInfos[bindingIndex] = {};
            for (size_t i = 0; i < bufferSizeInfo.second; ++i) {
                VkDescriptorBufferInfo info{};
                info.buffer = mesh.uniformBuffers.back()->getHandle();
                info.offset = offset;
                info.range = bufferSizeInfo.first;
                bufferInfos[bindingIndex][i] = info;

                offset += bufferSizeInfo.first;
            }
        }
    }

    for (const auto& [binding, textureInfos] : textureInfosMap) {
        mesh.textures[binding] = {};
        for (const auto& [arrayIndex, info] : textureInfos) {
            mesh.textures[binding].emplace(arrayIndex, &requireTexture(info.filepath, info.sampler));
        }
    }

    BindingMap<VkDescriptorImageInfo> imageInfos{};
    for (const auto& [binding, textureArray] : mesh.textures) {
        imageInfos[binding] = {};
        for (const auto& [arrayIndex, texture] : textureArray) {
            imageInfos[binding][arrayIndex] = texture->getImageInfo();
        }
    }

    for (uint32_t i = 0; i < threadCount; ++i)
    {
        mesh.descriptorSets.push_back(&requireDescriptorSet(descSetLayout, bufferInfos, imageInfos));
    }

    meshes.emplace_back(std::move(mesh));
    return meshes.size() - 1;
}

RenderMeshID VulkanResourceManager::requireRenderMesh(
    const std::vector<Vertex>& vertices, 
    const std::vector<uint32_t>& indices, 
    const GltfMaterial& mat, 
    const std::vector<RenderTexture> textures)
{
    RenderMesh mesh{};

    auto texturedMat = mat;
    auto setTexture = [&](int& textureId)
    {
        if (textureId > -1) {
            auto& tex = textures[textureId];
            requireTexture(tex.filepath, tex.sampler);
            textureId = textures.size() - 1;
        }
    };
    {
        setTexture(texturedMat.pbrBaseColorTexture);
        setTexture(texturedMat.pbrMetallicRoughnessTexture);
        setTexture(texturedMat.normalTexture);
        setTexture(texturedMat.occlusionTexture);
        setTexture(texturedMat.emissiveTexture);
        setTexture(texturedMat.transmissionTexture);
        setTexture(texturedMat.khrDiffuseTexture);
        setTexture(texturedMat.khrSpecularGlossinessTexture);
        setTexture(texturedMat.clearcoatTexture);
        setTexture(texturedMat.clearcoatRoughnessTexture);
    }

    std::vector<int32_t> matIndices(indices.size() / 3, 0);

    VkBufferUsageFlags flag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkBufferUsageFlags rayTracingFlags = // used also for building acceleration structures 
        flag | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    auto& vertexBuffer = requireBufferWithData(vertices,
        rayTracingFlags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    auto& indexBuffer = requireBufferWithData(indices,
        rayTracingFlags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    auto& matBuffer = requireBufferWithData(&texturedMat, sizeof(texturedMat), 
        flag | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    auto& matIndicesBuffer = requireBufferWithData(matIndices,
        flag | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    mesh.vertexBuffer = vertexBuffer.getBufferInfo();
    mesh.vertexNum = toU32(vertices.size());

    mesh.indexBuffer = indexBuffer.getBufferInfo();
    mesh.indexNum = toU32(indices.size());
    mesh.indexType = VK_INDEX_TYPE_UINT32;

    mesh.matBuffer = matBuffer.getBufferInfo();
    mesh.matIndicesBuffer = matIndicesBuffer.getBufferInfo();

    meshes.emplace_back(std::move(mesh));
    return meshes.size() - 1;
}

SceneData VulkanResourceManager::requireSceneData(const VulkanDescriptorSetLayout& descSetLayout, uint32_t threadCount,
    const std::map<uint32_t, std::pair<VkDeviceSize, size_t>>& bufferSizeInfos)
{
    SceneData sceneData{};
    sceneData.descSetLayout = &descSetLayout;
    sceneData.uniformBuffers.resize(threadCount);

    VkDeviceSize uniformBufferSize = 0;
    for (auto& [binding, bufferSizeInfo] : bufferSizeInfos) {
        if (descSetLayout.getType(binding) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            uniformBufferSize += bufferSizeInfo.first * bufferSizeInfo.second;
    }

    BindingMap<VkDescriptorBufferInfo> bufferInfos{};
    for (uint32_t threadIdx = 0; threadIdx < threadCount; ++threadIdx) {
        if (uniformBufferSize != 0) {
            auto& uniformBuffer = requireBuffer(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            sceneData.uniformBuffers[threadIdx].push_back(&uniformBuffer);
        }

        VkDeviceSize offset = 0;
        for (const auto& [bindingIndex, bufferSizeInfo] : bufferSizeInfos)
        {
            bufferInfos[bindingIndex] = {};
            if (descSetLayout.getType(bindingIndex) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                auto& uniformBuffer = sceneData.uniformBuffers[threadIdx][0];
                for (size_t i = 0; i < bufferSizeInfo.second; ++i) {
                    VkDescriptorBufferInfo info{};
                    info.buffer = uniformBuffer->getHandle();
                    info.offset = offset;
                    info.range = bufferSizeInfo.first;
                    bufferInfos[bindingIndex][i] = info;

                    offset += bufferSizeInfo.first;
                }
            }
            else {
                auto& storageBuffer = requireBuffer(bufferSizeInfo.first, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                sceneData.uniformBuffers[threadIdx].push_back(&storageBuffer);

                VkDescriptorBufferInfo info{};
                info.buffer = storageBuffer.getHandle();
                info.offset = 0;
                info.range = bufferSizeInfo.first;
                bufferInfos[bindingIndex][0] = info;
            }
        }
    }

    for (uint32_t i = 0; i < threadCount; ++i)
    {
        sceneData.descriptorSets.push_back(&requireDescriptorSet(descSetLayout, bufferInfos, {}));
    }

    return sceneData;
}

VulkanBuffer& VulkanResourceManager::requireBufferWithData(const void* data, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    auto stagingBuffer = std::make_unique<VulkanBuffer>(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer->update(data, bufferSize);
    auto& buffer = requireBuffer(bufferSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, properties);
    commandPool.copyBuffer(*stagingBuffer, buffer, bufferSize, device.getGraphicsQueue());

    return buffer;
}

VulkanBuffer& VulkanResourceManager::requireBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    auto buffer = new VulkanBuffer(device, bufferSize, usage, properties);
    bufferSet[buffer] = std::unique_ptr<VulkanBuffer>(buffer);
    return *buffer;
}

void VulkanResourceManager::destroyBuffer(VulkanBuffer* buffer)
{
    bufferSet.erase(buffer);
}

VulkanDescriptorSet& VulkanResourceManager::requireDescriptorSet(const VulkanDescriptorSetLayout& descSetLayout, const BindingMap<VkDescriptorBufferInfo>& bufferInfos, const BindingMap<VkDescriptorImageInfo>& imageInfos)
{
    auto descriptorSet = new VulkanDescriptorSet(device, *descriptorPools.front(), descSetLayout, bufferInfos, imageInfos);
    descriptorSetSet.emplace(descriptorSet);
    return *descriptorSet;
}

VulkanDescriptorPool& VulkanResourceManager::requireDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets)
{
    auto descriptorPool = new VulkanDescriptorPool(device, poolSizes, maxSets);
    descriptorPools.emplace_back(descriptorPool);
    return *descriptorPool;
}

VulkanTexture& VulkanResourceManager::requireTexture(const char* filename, VkSampler sampler)
{
    auto texture = new VulkanTexture(device, filename, sampler, commandPool, device.getGraphicsQueue());
    textureMap.emplace_back(texture);
    return *texture;
}

VkSampler VulkanResourceManager::createSampler(VkSamplerCreateInfo* createInfo)
{
    VkSampler sampler;

	if (createInfo == nullptr) {
        const VkPhysicalDeviceProperties& properties = device.getGPU().getProperties();

		VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.anisotropyEnable = VK_TRUE;
        info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        info.unnormalizedCoordinates = VK_FALSE;
        info.compareEnable = VK_FALSE;
        info.compareOp = VK_COMPARE_OP_ALWAYS;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.mipLodBias = 0.0f;
        info.minLod = 0.0f;
        info.maxLod = 100.0f;

        if (vkCreateSampler(device.getHandle(), &info, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
	}
    else {
        if (vkCreateSampler(device.getHandle(), createInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    samplerSet.emplace(sampler);

    return sampler;
}

VulkanShaderModule VulkanResourceManager::createShaderModule(const char* filepath, VkShaderStageFlagBits stageFlag, const char* name)
{
    auto shaderCode = readFile(filepath);

    return VulkanShaderModule(device, shaderCode, stageFlag, name);
}

BlasInput VulkanResourceManager::requireBlasInput(const RenderMesh& mesh)
{
    VkDeviceAddress vertexAddress = getBufferDeviceAddress(device.getHandle(), mesh.vertexBuffer.buffer);
    VkDeviceAddress indexAddress = getBufferDeviceAddress(device.getHandle(), mesh.indexBuffer.buffer);

    uint32_t maxPrimitiveCount = mesh.indexNum / 3;

    // Describe buffer as array of Vertex.
    VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
    triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 vertex position data.
    triangles.vertexData.deviceAddress = vertexAddress;
    triangles.vertexStride = sizeof(Vertex);
    triangles.indexType = mesh.indexType;
    triangles.indexData.deviceAddress = indexAddress;
    // Indicate identity transform by setting transformData to null device pointer.
    //triangles.transformData = {};
    triangles.maxVertex = mesh.vertexNum;

    VkAccelerationStructureGeometryKHR asGeom{};
    asGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    asGeom.geometry.triangles = triangles;

    // The entire array will be used to build the BLAS.
    VkAccelerationStructureBuildRangeInfoKHR offset;
    offset.firstVertex = 0;
    offset.primitiveCount = maxPrimitiveCount;
    offset.primitiveOffset = 0;
    offset.transformOffset = 0;

    // Our blas is made from only one geometry, but could be made of many geometries
    BlasInput input;
    input.asGeometry.emplace_back(asGeom);
    input.asBuildOffsetInfo.emplace_back(offset);
    input.mesh = &mesh;

    return input;
}

VulkanAccelerationStructure VulkanResourceManager::requireAS(VkAccelerationStructureCreateInfoKHR& info)
{
    VulkanAccelerationStructure as{};
    as.buffer = &requireBuffer(info.size,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    info.buffer = as.buffer->getHandle();

    VK_CHECK(vkCreateAccelerationStructureKHR(device.getHandle(), &info, nullptr, &as.handle));

    return as;
}

const std::vector<std::unique_ptr<VulkanTexture>>& VulkanResourceManager::getTextures() const
{
    return textureMap;
}

size_t VulkanResourceManager::getTextureNum() const
{
    return textureMap.size();
}

void SceneData::update() const
{
    for (auto& ds : descriptorSets)
        ds->update();
}
