#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_set>

#include "Vertex.h"
#include "Light.h"

#include "VulkanCommon.h"
#include "VulkanDescriptorSet.h"
#include "VulkanTexture.h"

// Information of a obj model when referenced in a shader
struct ObjDesc
{
    int      txtOffset;             // Texture index offset in the array of textures
    uint64_t vertexAddress;         // Address of the Vertex buffer
    uint64_t indexAddress;          // Address of the index buffer
    uint64_t materialAddress;       // Address of the material buffer
    uint64_t materialIndexAddress;  // Address of the triangle material index buffer
};

// Uniform buffer set at each frame
struct GlobalData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
};

struct ObjectData {
    glm::mat4 model;
};

struct SceneData {
    const VulkanDescriptorSetLayout* descSetLayout{ nullptr };
    std::vector<VulkanDescriptorSet*> descriptorSets;
    std::vector<std::vector<VulkanBuffer*>> uniformBuffers;

    void update() const;
};

struct RenderMaterial {
    glm::vec3  ambient;
    glm::vec3  diffuse;
    glm::vec3  specular;
    glm::vec3  transmittance;
    glm::vec3  emission;
    float shininess = 32.0f;
    float ior;       // index of refraction
    float dissolve = 1.0f;  // 1 == opaque; 0 == fully transparent
    int   diff_textureId = -1;
    int   spec_textureId = -1;
    int   norm_textureId = -1;
};

struct RenderTexture
{
    const char* filepath;
    VkSampler sampler;
};

struct RenderMesh
{
    VkIndexType indexType;
    uint32_t indexNum;
    uint32_t vertexNum;
    VkDescriptorBufferInfo vertexBuffer;
    VkDescriptorBufferInfo indexBuffer;
    VkDescriptorBufferInfo matBuffer;
    VkDescriptorBufferInfo matIndicesBuffer;

    glm::mat4 tranformMatrix{ 1.0f };

    VulkanPipeline* pipeline;
    const VulkanDescriptorSetLayout* descSetLayout;
    std::vector<VulkanDescriptorSet*> descriptorSets;
    BindingMap<VulkanTexture*> textures;
    std::vector<VulkanBuffer*> uniformBuffers;
};

struct RenderModel
{
    std::unique_ptr<VulkanDescriptorPool> descriptorPool;
    std::vector<RenderMesh> meshes;
};

using RenderMeshID = uint64_t;
using TextureID = uint64_t;

// raytracing
struct BlasInput
{
    // Data used to build acceleration structure geometry
    std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
    VkBuildAccelerationStructureFlagsKHR                  flags{ 0 };

    const RenderMesh* mesh;
};

struct VulkanAccelerationStructure
{
    VkAccelerationStructureKHR handle{ VK_NULL_HANDLE };
    VulkanBuffer* buffer{ nullptr };
};

class VulkanResourceManager
{
public:
	VulkanResourceManager(const VulkanDevice& device, VulkanCommandPool& commandPool);
	~VulkanResourceManager();

    RenderMeshID requireRenderMesh(
        const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const RenderMaterial& mat, 
        VulkanPipeline* pipeline, const VulkanDescriptorSetLayout& descSetLayout, uint32_t threadCount,
        const std::map<uint32_t, std::pair<VkDeviceSize, size_t>>& bufferSizeInfos, const BindingMap<RenderTexture>& textureInfosMap);

    RenderMeshID requireRenderMesh(
        const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, 
        const RenderMaterial& mat, const std::vector<RenderTexture> textures);

    SceneData requireSceneData(const VulkanDescriptorSetLayout& descSetLayout, uint32_t threadCount,
        const std::map<uint32_t, std::pair<VkDeviceSize, size_t>>& bufferSizeInfos);

    VulkanBuffer& requireBufferWithData(const void* data, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    VulkanBuffer& requireBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    
    template<class T>
    VulkanBuffer& requireBufferWithData(const std::vector<T>& vec, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        return requireBufferWithData(vec.data(), sizeof(T) * vec.size(), usage, properties);
    }

    void destroyBuffer(VulkanBuffer* buffer);

    VulkanDescriptorSetLayout& requireDescriptorSetLayout(uint32_t set, const std::vector<VulkanShaderResource>& shaderResources);
    VulkanDescriptorSet& requireDescriptorSet(const VulkanDescriptorSetLayout& descSetLayout, const BindingMap<VkDescriptorBufferInfo>& bufferInfos, const BindingMap<VkDescriptorImageInfo>& imageInfos);
    VulkanDescriptorPool& requireDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
    VulkanTexture& requireTexture(const char* filename, VkSampler sampler);

    VkSampler createSampler(VkSamplerCreateInfo* createInfo = nullptr);
    VulkanShaderModule createShaderModule(const char* filepath, VkShaderStageFlagBits stageFlag, const char* name);

    //raytracing
    BlasInput requireBlasInput(const RenderMesh& mesh);
    VulkanAccelerationStructure requireAS(VkAccelerationStructureCreateInfoKHR& info);

    const std::vector<std::unique_ptr<VulkanTexture>>& getTextures() const;
    size_t getTextureNum() const;

    inline RenderMesh& getRenderMesh(RenderMeshID id) {
        return meshes[id];
    };

    inline size_t getRenderMeshNum() const {
        return meshes.size();
    }

    inline const std::vector<RenderMesh> getRenderMeshes() const {
        return meshes;
    }

private:
    const VulkanDevice& device;
    VulkanCommandPool& commandPool;

    std::vector<RenderMesh> meshes;
    std::vector<std::unique_ptr<VulkanTexture>> textureMap;
    std::unordered_set<VkSampler> samplerSet;

    std::unordered_map<VulkanBuffer*, std::unique_ptr<VulkanBuffer>> bufferSet;

    std::vector<std::unique_ptr<VulkanDescriptorPool>> descriptorPools;
    std::unordered_set<std::unique_ptr<VulkanDescriptorSet>> descriptorSetSet;
};
