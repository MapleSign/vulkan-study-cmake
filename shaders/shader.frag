#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "host_device.h"

layout(push_constant) uniform PushConstants {
    PushConstantRaster constants;
};

#define MAX_POINT_LIGHT_NUM 16

layout(set = 1, binding = 0) uniform DirLightInfo {
    DirLight dirLight;
} dirLightInfo;

layout(set = 1, binding = 1) uniform PointLightInfo {
    PointLight pointLights[MAX_POINT_LIGHT_NUM];
} pointLightInfo;

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices { ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Materials { GltfMaterial m[]; }; // Array of all materials on an object
layout(buffer_reference, scalar) buffer MatIndices { int i[]; }; // Material ID for each triangle

layout(set = 0, binding = eObjDescs, scalar) buffer ObjDescBuffer { ObjDesc i[]; } objDesc;

layout(set = 0, binding = eTextures) uniform sampler2D[] textureSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in mat3 fragTBN;

layout(location = 0) out vec4 outColor;

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir, GltfMaterial mat);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, GltfMaterial mat);

void main() {
    ObjDesc objResource = objDesc.i[constants.objId];
    MatIndices matIndices = MatIndices(objResource.materialIndexAddress);
    Materials materials = Materials(objResource.materialAddress);

    int matIndex = matIndices.i[gl_PrimitiveID];
    GltfMaterial mat = materials.m[matIndex];

    // vec3 norm = normalize(fragNormal);
    vec3 norm = texture(textureSampler[mat.normalTexture], fragTexCoord).rgb;
    norm = normalize(norm * 2.0 - 1.0);
    norm = normalize(fragTBN * norm);

    vec3 viewDir = normalize(constants.viewPos - fragPos);

    vec3 result = calcDirLight(dirLightInfo.dirLight, norm, viewDir, mat);
    for (int i = 0; i < constants.lightNum; ++i) {
        result += calcPointLight(pointLightInfo.pointLights[i], norm, fragPos, viewDir, mat);    
    }

    outColor = vec4(result, 1.0);
    // outColor = vec4(1.0);
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir, GltfMaterial mat)
{
    vec3 lightDir = normalize(light.direction);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
//    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(textureSampler[mat.khrDiffuseTexture], fragTexCoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(textureSampler[mat.khrDiffuseTexture], fragTexCoord));
    vec3 specular = light.specular * spec * vec3(texture(textureSampler[mat.khrSpecularGlossinessTexture], fragTexCoord));
    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, GltfMaterial mat)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
//    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient = light.ambient * vec3(texture(textureSampler[mat.khrDiffuseTexture], fragTexCoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(textureSampler[mat.khrDiffuseTexture], fragTexCoord));
    vec3 specular = light.specular * spec * vec3(texture(textureSampler[mat.khrSpecularGlossinessTexture], fragTexCoord));
    
    return attenuation * (ambient + diffuse + specular);
}
