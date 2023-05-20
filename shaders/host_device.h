#ifndef COMMON_HOST_DEVICE
#define COMMON_HOST_DEVICE

#ifdef __cplusplus
#include <glm/glm.hpp>
// GLSL Type
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using uint = unsigned int;
#endif

// clang-format off
#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
#define START_BINDING(a) enum a {
#define END_BINDING() }
#else
#define START_BINDING(a)  const uint
#define END_BINDING() 
#endif

START_BINDING(SceneBindings)
eGlobals = 0,  // Global uniform containing camera matrices
eObjData = 1,  // Access to the object data
eObjDescs = 2,  // Access to the object descriptions
eTextures = 3  // Access to textures
END_BINDING();

START_BINDING(RtxBindings)
eTlas = 0,  // Top-level acceleration structure
eOutImage = 1   // Ray tracer output image
END_BINDING();
// clang-format on

struct DirLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

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
    mat4 view;
    mat4 proj;
    mat4 viewInverse;
    mat4 projInverse;
};

struct ObjectData {
    mat4 model;
};

// Push constant structure for the raster
struct PushConstantRaster
{
	vec3 viewPos;
	int objId;
	int lightNum;
};

// Push constant structure for the ray tracer
struct PushConstantRay
{
	vec4  clearColor;
	vec3  lightPosition;
	float lightIntensity;
	int   lightType;
	int   frame;
    int   maxDepth;
	int   sampleNumbers;
};

struct Vertex  // See ObjLoader, copy of VertexObj, could be compressed for device
{
	vec3 pos;
	vec3 normal;
	vec2 texCoord;
	vec3 tangent;
	vec3 bitangent;
};

#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2

struct GltfMaterial  // GLTF Material
{
	// 0
    vec4 pbrBaseColorFactor;
    // 4
    int   pbrBaseColorTexture;
    float pbrMetallicFactor;
    float pbrRoughnessFactor;
    int   pbrMetallicRoughnessTexture;
    // 8
    vec4 khrDiffuseFactor;  // KHR_materials_pbrSpecularGlossiness
    vec3 khrSpecularFactor;
    int  khrDiffuseTexture;
    // 16
    int   shadingModel;  // 0: metallic-roughness, 1: specular-glossiness
    float khrGlossinessFactor;
    int   khrSpecularGlossinessTexture;
    int   emissiveTexture;
    // 20
    vec3 emissiveFactor;
    int  alphaMode;
    // 24
    float alphaCutoff;
    int   doubleSided;
    int   normalTexture;
    float normalTextureScale;
    // 28
    int occlusionTexture;
    float occlusionTextureStrength;
    float transmissionFactor;
    int   transmissionTexture;
    // 32
    float clearcoatFactor;
    float clearcoatRoughness;
    int  clearcoatTexture;
    int  clearcoatRoughnessTexture;
    //36
    float ior;
    vec3 pad;
};


#endif
