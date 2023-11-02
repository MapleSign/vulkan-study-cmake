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

layout(set = 1, binding = 0) buffer DirLightInfo {
    DirLight dirLight[];
};

layout(set = 1, binding = 1) buffer PointLightInfo {
    PointLight pointLights[];
};

layout(set = 1, binding = 2) uniform _ShadowUniform {
    ShadowData shadowUniform;
};

layout(set = 1, binding = 3) uniform sampler2D[] dirLightShadowMaps;
layout(set = 1, binding = 4) uniform samplerCube[] pointLightShadowMaps;

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices { ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Materials { GltfMaterial m[]; }; // Array of all materials on an object
layout(buffer_reference, scalar) buffer MatIndices { int i[]; }; // Material ID for each triangle

layout(set = 0, binding = eObjDescs, scalar) buffer ObjDescBuffer { ObjDesc i[]; } objDesc;

layout(set = 0, binding = eTextures) uniform sampler2D[] textureSampler;
layout(set = 0, binding = eEnvTexture) uniform samplerCube envSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

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

float calcDirShadow(sampler2D shadowMap, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z - shadowUniform.bias;
    float shadow = 0.0;

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

float calcCubeShadow(samplerCube shadowMap, vec3 fragPos, vec3 lightPos) {
    vec3 fragToLight = fragPos - lightPos;
    float closestDepth = texture(shadowMap, fragToLight).r;
    closestDepth *= 25.0;
    float currentDepth = length(fragToLight) - shadowUniform.bias;
    float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

    return shadow;
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
    state.ffnormal = dot(state.normal, viewDir) >= 0.0 ? state.normal : -state.normal;
    createCoordinateSystem(state.ffnormal, state.tangent, state.bitangent);
    
    vec3 result = vec3(0.0);
    for (int i = 0; i < constants.dirLightNum; ++i) {
        vec3 lightDir = -normalize(dirLight[i].direction);
        vec3 lightIntensity = dirLight[i].intensity * dirLight[i].color;

        vec4 fragPosLightSpace = dirLight[i].lightSpace * vec4(fragPos, 1.0);
        float shadow = i < shadowUniform.maxDirShadowNum ? calcDirShadow(dirLightShadowMaps[nonuniformEXT(i)], fragPosLightSpace) : 0.0;

        result += (1.0 - shadow) * calcLight(state, viewDir, lightDir, lightIntensity, 1.0);
    }
    
    for (int i = 0; i < constants.pointLightNum; ++i) {
        PointLight light = pointLights[i];
        vec3 lightDir = normalize(light.position - fragPos);
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
        vec3 lightIntensity = light.color * light.intensity * attenuation;
        
        float shadow = i < shadowUniform.maxPointShadowNum ? calcCubeShadow(pointLightShadowMaps[nonuniformEXT(i)], fragPos, light.position) : 0.0;

        result += (1.0 - shadow) * calcLight(state, viewDir, lightDir, lightIntensity, 1.0);
    }

    int numSamples = 64;
    BsdfSampleRec indirectBsdf;
    uint seed = constants.objId;
    vec3 sampleColor = vec3(0);
    for (int i = 0; i < numSamples; ++i) {
        indirectBsdf.f = PbrSample(state, viewDir, state.ffnormal, indirectBsdf.L, indirectBsdf.pdf, seed);
        vec3 sampleLight = texture(envSampler, indirectBsdf.L).rgb;
        sampleColor += indirectBsdf.f * sampleLight * abs(dot(state.ffnormal, indirectBsdf.L)) / indirectBsdf.pdf;
    }
    sampleColor /= numSamples;
    result += sampleColor * 0.05;

    outColor = vec4(result + state.mat.emission, 1.0);
    // outColor = vec4(vec3(shadow), 1.0);
    // outColor = vec4((state.normal + vec3(1)) * 0.5, 1.0);
    // outColor = vec4(state.texCoord, 0, 1);
    // outColor = vec4(1.0);
}
