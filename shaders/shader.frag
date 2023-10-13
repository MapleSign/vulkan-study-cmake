#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#extension GL_ARB_shader_clock : enable

#include "raycommon.glsl"
#include "random.glsl"
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
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;
layout(location = 5) in mat3 fragTBN; // assigned multiple location 5,6,7

layout(location = 0) out vec4 outColor;

#include "gltf_material.glsl"
#include "pbr.glsl"

vec3 calcLight(inout State state, vec3 V, vec3 L, vec3 lightIntensity, float lightPdf) {
    vec3 Li = vec3(0);
    if (dot(state.ffnormal, L) > 0) {
        BsdfSampleRec directBsdf;
        directBsdf.f = PbrEval(state, V, state.ffnormal, L, directBsdf.pdf);
        float misWeightBsdf = max(0, powerHeuristic(directBsdf.pdf, lightPdf)); // multi importance sampling weight
        Li = misWeightBsdf * directBsdf.f * abs(dot(L, state.ffnormal)) * lightIntensity / directBsdf.pdf;
    }
    return Li;
}

void main() {
    ObjDesc objResource = objDesc.i[constants.objId];
    MatIndices matIndices = MatIndices(objResource.materialIndexAddress);
    Materials materials = Materials(objResource.materialAddress);

    int matIndex = matIndices.i[gl_PrimitiveID];
    GltfMaterial mat = materials.m[matIndex];

    State state;
    state.position = fragPos;
    state.normal = normalize(fragNormal);
    state.texCoord = fragTexCoord;
    state.tangent = normalize(fragTangent);
    state.bitangent = normalize(fragBitangent);
    getMaterialsAndTextures(state, mat);
    
    if(mat.alphaMode == ALPHA_MASK && state.mat.alpha < mat.alphaCutoff) {
        discard;
    }

    vec3 viewDir = normalize(constants.viewPos - fragPos);
    vec3 lightDir = -normalize(dirLightInfo.dirLight.direction);
    state.ffnormal = dot(state.normal, viewDir) >= 0.0 ? state.normal : -state.normal;
    
    vec3 result = calcLight(state, viewDir, lightDir, dirLightInfo.dirLight.diffuse, 1.0);
    
    for (int i = 0; i < constants.lightNum; ++i) {
        PointLight light = pointLightInfo.pointLights[i];
        lightDir = normalize(light.position - fragPos);
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
        result += calcLight(state, viewDir, lightDir, light.diffuse * attenuation, 1.0);    
    }

    outColor = vec4(result + state.mat.emission, 1.0);
    // outColor = vec4((state.normal + vec3(1)) * 0.5, 1.0);
    // outColor = vec4(state.texCoord, 0, 1);
    // outColor = vec4(1.0);
}
